"""
Holocubic Now Playing Companion

Windows 上執行，透過 Windows Media Session (SMTC) 偵測正在播放的音樂，
在 PC 端預渲染捲動動畫幀，上傳到 ESP32 SD 卡播放。

適用於所有 Windows 媒體來源：YouTube Music (瀏覽器)、Spotify、VLC 等。

用法:
    python now_playing.py <ESP32_IP>
    python now_playing.py 192.168.1.100 --interval 3

需求:
    pip install -r requirements.txt
"""

import argparse
import asyncio
import io
import os
import sys
import time

import requests
import unicodedata
from PIL import Image, ImageDraw, ImageFont

from winrt.windows.media.control import (
    GlobalSystemMediaTransportControlsSessionManager as MediaManager,
)
from winrt.windows.storage.streams import DataReader

# Windows Media PlaybackStatus enum values
PLAYBACK_PLAYING = 4

# Canvas size (must match ESP32 config)
CANVAS_SIZE = 128

# Art region (top portion)
ART_HEIGHT = 100

# Text bar region (bottom portion)
TEXT_BAR_HEIGHT = CANVAS_SIZE - ART_HEIGHT

# Scrolling
SCROLL_PX_PER_FRAME = 10
SCROLL_PAUSE_FRAMES = 15  # pause at start/end

# Font sizes
TITLE_FONT_SIZE = 10
ARTIST_FONT_SIZE = 8

# HTTP timeouts (seconds)
HTTP_TIMEOUT = 5
FRAME_UPLOAD_TIMEOUT = 10

# Preferred CJK fonts (Windows)
CJK_FONT_CANDIDATES = [
    "msjh.ttc",       # 微軟正黑體
    "msyh.ttc",       # 微軟雅黑
    "yugothm.ttc",    # Yu Gothic
    "NotoSansCJK-Regular.ttc",
    "arial.ttf",      # fallback
]


def _find_font(size):
    """Find a CJK-capable font on the system."""
    # Try Windows font directory
    windir = os.environ.get("WINDIR", r"C:\Windows")
    font_dir = os.path.join(windir, "Fonts")

    for name in CJK_FONT_CANDIDATES:
        path = os.path.join(font_dir, name)
        if os.path.exists(path):
            return ImageFont.truetype(path, size)

    # Last resort — Pillow default
    try:
        return ImageFont.truetype("arial.ttf", size)
    except OSError:
        return ImageFont.load_default()


# Pre-load fonts
TITLE_FONT = _find_font(TITLE_FONT_SIZE)
ARTIST_FONT = _find_font(ARTIST_FONT_SIZE)


def _text_width(text, font):
    """Measure text pixel width."""
    bbox = font.getbbox(text)
    return bbox[2] - bbox[0]


def _render_frames(art_img, title, artist):
    """
    Render all animation frames.
    Returns list of BMP bytes.

    Layout:
      - Top 100px: album art (or dark gradient)
      - Bottom 28px: semi-transparent bar with title + artist
    """
    frames = []

    # Prepare art (100x128, centered)
    if art_img:
        w, h = art_img.size
        if w != h:
            s = min(w, h)
            left = (w - s) // 2
            top = (h - s) // 2
            art_img = art_img.crop((left, top, left + s, top + s))
        art_img = art_img.resize((CANVAS_SIZE, CANVAS_SIZE), Image.LANCZOS)
    else:
        # Dark background when no art
        art_img = Image.new("RGB", (CANVAS_SIZE, CANVAS_SIZE), (20, 20, 30))

    # Measure text
    title_w = _text_width(title, TITLE_FONT)
    artist_w = _text_width(artist, ARTIST_FONT)
    max_visible = CANVAS_SIZE - 8  # 4px padding each side

    # Determine scroll ranges
    title_scroll = max(0, title_w - max_visible)
    artist_scroll = max(0, artist_w - max_visible)
    max_scroll = max(title_scroll, artist_scroll)

    if max_scroll == 0:
        # Static — single frame
        frame = _compose_frame(art_img, title, artist, 0, 0)
        frames.append(_to_bmp(frame))
    else:
        # Pause at start
        frame0 = _compose_frame(art_img, title, artist, 0, 0)
        bmp0 = _to_bmp(frame0)
        frames.append(bmp0)

        # Scroll
        total_scroll_steps = max_scroll // SCROLL_PX_PER_FRAME
        for step in range(1, total_scroll_steps + 1):
            px = step * SCROLL_PX_PER_FRAME
            t_off = min(px, title_scroll)
            a_off = min(px, artist_scroll)
            frame = _compose_frame(art_img, title, artist, t_off, a_off)
            frames.append(_to_bmp(frame))

        # Pause at end
        frame_end = _compose_frame(art_img, title, artist, title_scroll, artist_scroll)
        bmp_end = _to_bmp(frame_end)
        frames.append(bmp_end)

    return frames


def _compose_frame(art_img, title, artist, title_offset, artist_offset):
    """Compose a single 128x128 frame with art + text bar."""
    img = art_img.copy()
    draw = ImageDraw.Draw(img)

    # Semi-transparent dark bar at bottom
    bar_y = ART_HEIGHT
    for y in range(bar_y, CANVAS_SIZE):
        for x in range(CANVAS_SIZE):
            r, g, b = img.getpixel((x, y))
            img.putpixel((x, y), (r // 3, g // 3, b // 3))

    # Title (white)
    title_y = bar_y + 2
    title_x = 4 - title_offset
    draw.text((title_x, title_y), title, font=TITLE_FONT, fill=(255, 255, 255))

    # Artist (cyan)
    artist_y = bar_y + 16
    artist_x = 4 - artist_offset
    draw.text((artist_x, artist_y), artist, font=ARTIST_FONT, fill=(0, 210, 255))

    # Clip text bar side edges (redraw art edges to mask overflow)
    # Left mask
    for y in range(bar_y, CANVAS_SIZE):
        for x in range(0, 3):
            r, g, b = art_img.getpixel((x, y))
            img.putpixel((x, y), (r // 3, g // 3, b // 3))
    # Right mask
    for y in range(bar_y, CANVAS_SIZE):
        for x in range(CANVAS_SIZE - 3, CANVAS_SIZE):
            r, g, b = art_img.getpixel((x, y))
            img.putpixel((x, y), (r // 3, g // 3, b // 3))

    return img


def _to_bmp(img):
    """Convert PIL Image to BMP bytes."""
    out = io.BytesIO()
    img.save(out, format="BMP")
    return out.getvalue()


class NowPlayingCompanion:
    def __init__(self, ip, interval=2.0):
        self._ip = ip
        self._interval = interval
        self._last_key = ""
        self._running = True
        # Pending frames waiting to be uploaded
        self._pending_title = ""
        self._pending_artist = ""
        self._pending_frames = None  # list of BMP bytes, or None
        self._uploaded = False

    async def run(self):
        print(f"[Companion] Monitoring media -> http://{self._ip}")
        print(f"[Companion] Poll interval: {self._interval}s")
        print(f"[Companion] Title font: {TITLE_FONT.getname()}")
        print("[Companion] Press Ctrl+C to stop\n")

        while self._running:
            try:
                await self._poll()
            except Exception as e:
                print(f"[Companion] Error: {e}")
            await asyncio.sleep(self._interval)

    async def _poll(self):
        manager = await MediaManager.request_async()
        session = manager.get_current_session()
        if session is None:
            return

        # Only push when actively playing
        playback = session.get_playback_info()
        if playback.playback_status != PLAYBACK_PLAYING:
            return

        props = await session.try_get_media_properties_async()
        title = props.title or ""
        artist = props.artist or ""
        
        if not title:
            return

        title = unicodedata.normalize("NFC", title)
        artist = unicodedata.normalize("NFC", artist)

        # Deduplicate — detect new track
        key = f"{title}\0{artist}"
        if key != self._last_key:
            self._last_key = key
            print(f"  Now: {artist} - {title}")

            # Read album art
            art_bytes = await self._read_thumbnail(props.thumbnail)
            art_img = None
            if art_bytes:
                try:
                    art_img = Image.open(io.BytesIO(art_bytes)).convert("RGB")
                except Exception:
                    art_img = None

            # Render frames on PC
            frames = _render_frames(art_img, title, artist)
            print(f"    Rendered {len(frames)} frames")

            # Store pending frames
            self._pending_title = title
            self._pending_artist = artist
            self._pending_frames = frames
            self._uploaded = False

            # Push metadata to ESP32 (no SD operations, no _isUploading)
            ok = self._push_metadata(title, artist, len(frames))
            if not ok:
                self._last_key = ""  # retry next poll

        # Try to upload pending frames if NowPlaying app is active
        if self._pending_frames and not self._uploaded:
            if self._is_now_playing_active():
                print("    NowPlaying active, uploading frames...")
                ok = self._upload_frames(self._pending_frames)
                if ok:
                    print("    Uploaded OK")
                    self._uploaded = True
                else:
                    print("    Upload failed, will retry")

    # -- Windows Media Session ------------------------------------------------

    @staticmethod
    async def _read_thumbnail(thumbnail):
        """Read album art from Windows Media Session thumbnail stream."""
        if thumbnail is None:
            return None
        try:
            stream = await thumbnail.open_read_async()
            size = stream.size
            if size == 0:
                return None
            reader = DataReader(stream.get_input_stream_at(0))
            await reader.load_async(size)
            buf = bytearray(size)
            reader.read_bytes(buf)
            return bytes(buf)
        except Exception:
            return None

    # -- ESP32 HTTP API -------------------------------------------------------

    def _push_metadata(self, title, artist, frame_count):
        """Send track metadata to ESP32 (no frame upload yet)."""
        try:
            r = requests.post(
                f"http://{self._ip}/api/now-playing",
                json={
                    "title": title,
                    "artist": artist,
                    "frameCount": frame_count,
                },
                timeout=HTTP_TIMEOUT,
            )
            if not r.ok:
                print(f"    metadata failed: {r.status_code}")
                return False
            return True
        except requests.RequestException as e:
            print(f"    metadata error: {e}")
            return False

    def _is_now_playing_active(self):
        """Check if ESP32 is currently running the NowPlaying app."""
        try:
            r = requests.get(
                f"http://{self._ip}/api/mode",
                timeout=HTTP_TIMEOUT,
            )
            if r.ok:
                data = r.json()
                current = data.get("current", -1)
                apps = data.get("apps", [])
                if current >= 0 and current < len(apps):
                    return apps[current] == "NowPlaying"
        except requests.RequestException:
            pass
        return False

    def _upload_frames(self, frames):
        """Upload pre-rendered BMP frames to ESP32 SD card."""
        base = f"http://{self._ip}"

        # Upload each frame (with retry)
        for i, bmp in enumerate(frames):
            ok = False
            for attempt in range(3):
                try:
                    r = requests.post(
                        f"{base}/api/np/frame/{i}",
                        files={"file": (f"{i}.bmp", bmp, "image/bmp")},
                        timeout=FRAME_UPLOAD_TIMEOUT,
                    )
                    if r.ok:
                        ok = True
                        break
                    print(f"    Frame {i} attempt {attempt+1} failed: {r.status_code}")
                except requests.RequestException as e:
                    print(f"    Frame {i} attempt {attempt+1} error: {e}")
                time.sleep(0.5)
            if not ok:
                print(f"    Frame {i} upload failed after 3 attempts")
                return False

        # Signal frames ready
        try:
            requests.post(f"{base}/api/np/ready", timeout=HTTP_TIMEOUT)
        except requests.RequestException as e:
            print(f"    Frames ready error: {e}")

        return True

    def _set_mode(self, app_index):
        """Switch ESP32 app mode (0=GIF, 1=NowPlaying)."""
        try:
            requests.post(
                f"http://{self._ip}/api/mode",
                json={"app": app_index},
                timeout=HTTP_TIMEOUT,
            )
        except requests.RequestException:
            pass

    def stop(self):
        self._running = False


def main():
    parser = argparse.ArgumentParser(
        description="Holocubic Now Playing — push currently playing music to ESP32"
    )
    parser.add_argument("ip", help="ESP32 IP address (e.g. 192.168.1.100)")
    parser.add_argument(
        "--interval",
        type=float,
        default=2.0,
        help="Poll interval in seconds (default: 2.0)",
    )
    parser.add_argument(
        "--gif-on-exit",
        action="store_true",
        help="Switch back to GIF mode when script exits",
    )
    args = parser.parse_args()

    companion = NowPlayingCompanion(args.ip, args.interval)

    try:
        asyncio.run(companion.run())
    except KeyboardInterrupt:
        print("\n[Companion] Stopped.")
        if args.gif_on_exit:
            print("[Companion] Switching back to GIF mode...")
            companion._set_mode(0)


if __name__ == "__main__":
    main()
