#ifndef WEB_HTML_H
#define WEB_HTML_H

#include <Arduino.h>

// HTML content
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-TW">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Holocubic Manager</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        :root {
            --bg-primary: #ffffff;
            --bg-secondary: #f5f5f7;
            --bg-tertiary: #e8e8ed;
            --text-primary: #1d1d1f;
            --text-secondary: #86868b;
            --accent: #0071e3;
            --accent-hover: #0077ed;
            --border: #d2d2d7;
            --danger: #ff3b30;
            --danger-hover: #ff453a;
            --success: #34c759;
            --shadow: rgba(0, 0, 0, 0.04);
            --shadow-hover: rgba(0, 0, 0, 0.08);
            --radius: 12px;
            --radius-sm: 8px;
        }
        
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'SF Pro Text', 'Segoe UI', Roboto, sans-serif;
            background: var(--bg-secondary);
            color: var(--text-primary);
            line-height: 1.5;
            min-height: 100vh;
        }
        
        .container {
            max-width: 960px;
            margin: 0 auto;
            padding: 32px 24px;
        }
        
        header {
            text-align: center;
            margin-bottom: 40px;
        }
        
        h1 {
            font-size: 32px;
            font-weight: 600;
            letter-spacing: -0.5px;
            margin-bottom: 8px;
        }
        
        .subtitle {
            color: var(--text-secondary);
            font-size: 15px;
        }
        
        .card {
            background: var(--bg-primary);
            border-radius: var(--radius);
            box-shadow: 0 1px 3px var(--shadow);
            margin-bottom: 24px;
            overflow: hidden;
        }
        
        .card-header {
            padding: 20px 24px;
            border-bottom: 1px solid var(--bg-tertiary);
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
        
        .card-title {
            font-size: 17px;
            font-weight: 600;
        }
        
        .card-body {
            padding: 24px;
        }
        
        .upload-zone {
            border: 2px dashed var(--border);
            border-radius: var(--radius);
            padding: 48px 24px;
            text-align: center;
            cursor: pointer;
            transition: all 0.2s ease;
        }
        
        .upload-zone:hover, .upload-zone.dragover {
            border-color: var(--accent);
            background: rgba(0, 113, 227, 0.04);
        }
        
        .upload-zone svg {
            width: 48px;
            height: 48px;
            stroke: var(--text-secondary);
            margin-bottom: 16px;
        }
        
        .upload-zone p {
            color: var(--text-secondary);
            font-size: 15px;
        }
        
        .upload-zone .hint {
            font-size: 13px;
            margin-top: 8px;
        }
        
        input[type="file"] {
            display: none;
        }
        
        .btn {
            display: inline-flex;
            align-items: center;
            justify-content: center;
            gap: 6px;
            padding: 10px 20px;
            font-size: 14px;
            font-weight: 500;
            border: none;
            border-radius: var(--radius-sm);
            cursor: pointer;
            transition: all 0.2s ease;
        }
        
        .btn-primary {
            background: var(--accent);
            color: white;
        }
        
        .btn-primary:hover {
            background: var(--accent-hover);
        }
        
        .btn-secondary {
            background: var(--bg-tertiary);
            color: var(--text-primary);
        }
        
        .btn-secondary:hover {
            background: var(--border);
        }
        
        .btn-danger {
            background: transparent;
            color: var(--danger);
        }
        
        .btn-danger:hover {
            background: rgba(255, 59, 48, 0.1);
        }
        
        .btn-icon {
            padding: 8px;
            background: transparent;
            border-radius: var(--radius-sm);
        }
        
        .btn-icon:hover {
            background: var(--bg-tertiary);
        }
        
        .btn-icon svg {
            width: 20px;
            height: 20px;
            stroke: var(--text-secondary);
        }
        
        .gif-list {
            list-style: none;
        }
        
        .gif-item {
            display: flex;
            align-items: center;
            padding: 16px 24px;
            border-bottom: 1px solid var(--bg-tertiary);
            transition: background 0.15s ease;
        }
        
        .gif-item:last-child {
            border-bottom: none;
        }
        
        .gif-item:hover {
            background: var(--bg-secondary);
        }
        
        .gif-item.dragging {
            opacity: 0.5;
            background: var(--bg-tertiary);
        }
        
        .drag-handle {
            cursor: grab;
            padding: 8px;
            margin-right: 12px;
            color: var(--text-secondary);
        }
        
        .drag-handle:active {
            cursor: grabbing;
        }
        
        .drag-handle svg {
            width: 16px;
            height: 16px;
            stroke: currentColor;
        }
        
        .gif-preview {
            width: 64px;
            height: 64px;
            border-radius: var(--radius-sm);
            background: var(--bg-tertiary);
            margin-right: 16px;
            overflow: hidden;
            display: flex;
            align-items: center;
            justify-content: center;
        }
        
        .gif-preview img {
            max-width: 100%;
            max-height: 100%;
            object-fit: contain;
        }
        
        .gif-info {
            flex: 1;
        }
        
        .gif-name {
            font-weight: 500;
            font-size: 15px;
            margin-bottom: 4px;
        }
        
        .gif-meta {
            font-size: 13px;
            color: var(--text-secondary);
        }
        
        .gif-actions {
            display: flex;
            gap: 4px;
        }
        
        .empty-state {
            text-align: center;
            padding: 48px 24px;
            color: var(--text-secondary);
        }
        
        .empty-state svg {
            width: 64px;
            height: 64px;
            stroke: var(--border);
            margin-bottom: 16px;
        }
        
        .progress-overlay {
            position: fixed;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background: rgba(255, 255, 255, 0.9);
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            z-index: 1000;
            opacity: 0;
            visibility: hidden;
            transition: all 0.3s ease;
        }
        
        .progress-overlay.active {
            opacity: 1;
            visibility: visible;
        }
        
        .progress-spinner {
            width: 48px;
            height: 48px;
            border: 3px solid var(--bg-tertiary);
            border-top-color: var(--accent);
            border-radius: 50%;
            animation: spin 1s linear infinite;
            margin-bottom: 16px;
        }
        
        @keyframes spin {
            to { transform: rotate(360deg); }
        }
        
        .progress-text {
            font-size: 15px;
            color: var(--text-secondary);
        }
        
        .progress-bar {
            width: 200px;
            height: 4px;
            background: var(--bg-tertiary);
            border-radius: 2px;
            margin-top: 12px;
            overflow: hidden;
        }
        
        .progress-bar-fill {
            height: 100%;
            background: var(--accent);
            width: 0%;
            transition: width 0.3s ease;
        }
        
        .modal {
            position: fixed;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background: rgba(0, 0, 0, 0.4);
            display: flex;
            align-items: center;
            justify-content: center;
            z-index: 1001;
            opacity: 0;
            visibility: hidden;
            transition: all 0.3s ease;
        }
        
        .modal.active {
            opacity: 1;
            visibility: visible;
        }
        
        .modal-content {
            background: var(--bg-primary);
            border-radius: var(--radius);
            padding: 24px;
            max-width: 400px;
            width: 90%;
            transform: scale(0.95);
            transition: transform 0.3s ease;
        }
        
        .modal.active .modal-content {
            transform: scale(1);
        }
        
        .modal-title {
            font-size: 17px;
            font-weight: 600;
            margin-bottom: 12px;
        }
        
        .modal-body {
            color: var(--text-secondary);
            font-size: 15px;
            margin-bottom: 24px;
        }
        
        .modal-actions {
            display: flex;
            gap: 12px;
            justify-content: flex-end;
        }
        
        .preview-modal .modal-content {
            max-width: 320px;
        }
        
        .preview-container {
            background: var(--bg-tertiary);
            border-radius: var(--radius-sm);
            padding: 24px;
            display: flex;
            align-items: center;
            justify-content: center;
            margin-bottom: 16px;
        }
        
        .preview-container canvas {
            max-width: 100%;
            image-rendering: pixelated;
        }
        
        .toast {
            position: fixed;
            bottom: 24px;
            left: 50%;
            transform: translateX(-50%) translateY(100px);
            background: var(--text-primary);
            color: white;
            padding: 12px 24px;
            border-radius: var(--radius-sm);
            font-size: 14px;
            opacity: 0;
            transition: all 0.3s ease;
            z-index: 1002;
        }
        
        .toast.show {
            transform: translateX(-50%) translateY(0);
            opacity: 1;
        }
        
        @media (max-width: 640px) {
            .container {
                padding: 20px 16px;
            }
            
            h1 {
                font-size: 24px;
            }
            
            .card-header {
                flex-direction: column;
                gap: 12px;
                align-items: flex-start;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>Holocubic</h1>
            <p class="subtitle">GIF Manager 繚 <a href="/wifi" style="color:var(--accent);text-decoration:none;">WiFi Settings</a></p>
        </header>
        
        <div class="card">
            <div class="card-header">
                <span class="card-title">App Mode</span>
                <span class="subtitle" id="currentApp">--</span>
            </div>
            <div class="card-body" style="display:flex;gap:8px;flex-wrap:wrap;" id="appButtons">
            </div>
        </div>
        
        <div class="card">
            <div class="card-header">
                <span class="card-title">Upload GIF</span>
            </div>
            <div class="card-body">
                <div class="upload-zone" id="uploadZone">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
                        <path d="M12 16V4m0 0L8 8m4-4l4 4"/>
                        <path d="M3 15v4a2 2 0 002 2h14a2 2 0 002-2v-4"/>
                    </svg>
                    <p>Drop GIF file here or click to browse</p>
                    <p class="hint">Supports animated GIF up to 100 frames</p>
                </div>
                <input type="file" id="fileInput" accept=".gif,image/gif">
            </div>
        </div>
        
        <div class="card">
            <div class="card-header">
                <span class="card-title">Your GIFs</span>
                <span class="subtitle" id="gifCount">0 items</span>
            </div>
            <ul class="gif-list" id="gifList">
                <li class="empty-state">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
                        <rect x="3" y="3" width="18" height="18" rx="2"/>
                        <circle cx="8.5" cy="8.5" r="1.5"/>
                        <path d="M21 15l-5-5L5 21"/>
                    </svg>
                    <p>No GIFs yet</p>
                </li>
            </ul>
        </div>
    </div>
    
    <div class="progress-overlay" id="progressOverlay">
        <div class="progress-spinner"></div>
        <p class="progress-text" id="progressText">Processing...</p>
        <div class="progress-bar">
            <div class="progress-bar-fill" id="progressFill"></div>
        </div>
    </div>
    
    <div class="modal" id="deleteModal">
        <div class="modal-content">
            <h3 class="modal-title">Delete GIF</h3>
            <p class="modal-body">Are you sure you want to delete "<span id="deleteGifName"></span>"? This action cannot be undone.</p>
            <div class="modal-actions">
                <button class="btn btn-secondary" onclick="closeDeleteModal()">Cancel</button>
                <button class="btn btn-primary" style="background: var(--danger);" onclick="confirmDelete()">Delete</button>
            </div>
        </div>
    </div>
    
    <div class="modal preview-modal" id="previewModal">
        <div class="modal-content">
            <h3 class="modal-title" id="previewTitle">Preview</h3>
            <div class="preview-container">
                <canvas id="previewCanvas" width="128" height="128"></canvas>
            </div>
            <div class="modal-actions">
                <button class="btn btn-secondary" onclick="closePreviewModal()">Close</button>
            </div>
        </div>
    </div>
    
    <div class="toast" id="toast"></div>
    
    <script>
        let gifs = [];
        let deleteTarget = null;
        let previewInterval = null;
        
        const uploadZone = document.getElementById('uploadZone');
        const fileInput = document.getElementById('fileInput');
        const gifList = document.getElementById('gifList');
        const gifCount = document.getElementById('gifCount');
        const progressOverlay = document.getElementById('progressOverlay');
        const progressText = document.getElementById('progressText');
        const progressFill = document.getElementById('progressFill');
        
        // Upload zone events
        uploadZone.addEventListener('click', () => fileInput.click());
        uploadZone.addEventListener('dragover', (e) => {
            e.preventDefault();
            uploadZone.classList.add('dragover');
        });
        uploadZone.addEventListener('dragleave', () => {
            uploadZone.classList.remove('dragover');
        });
        uploadZone.addEventListener('drop', (e) => {
            e.preventDefault();
            uploadZone.classList.remove('dragover');
            if (e.dataTransfer.files.length) {
                handleFile(e.dataTransfer.files[0]);
            }
        });
        fileInput.addEventListener('change', (e) => {
            if (e.target.files.length) {
                handleFile(e.target.files[0]);
            }
        });
        
        // GIF parsing and upload
        const MAX_SIZE = 128;
        const MAX_NAME_LEN = 32;
        
        async function handleFile(file) {
            if (!file.type.includes('gif')) {
                showToast('Please select a GIF file');
                return;
            }
            
            showProgress('Reading GIF...');
            
            try {
                const arrayBuffer = await file.arrayBuffer();
                const frames = await parseGif(new Uint8Array(arrayBuffer));
                
                if (frames.length === 0) {
                    hideProgress();
                    showToast('Could not parse GIF');
                    return;
                }
                
                // Scale frames if needed
                updateProgress('Scaling frames...', 5);
                const scaledFrames = scaleFrames(frames, MAX_SIZE);
                
                let gifName = file.name.replace(/\.gif$/i, '').replace(/[^a-zA-Z0-9_-]/g, '_');
                if (gifName.length > MAX_NAME_LEN) gifName = gifName.substring(0, MAX_NAME_LEN);
                
                // Calculate average delay for defaultDelay
                const totalDelay = scaledFrames.reduce((sum, f) => sum + f.delay, 0);
                const defaultDelay = Math.round(totalDelay / scaledFrames.length);
                
                // Create GIF entry
                updateProgress('Creating GIF...', 10);
                const createRes = await fetch('/api/gif', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({
                        name: gifName,
                        frameCount: scaledFrames.length,
                        width: scaledFrames[0].width,
                        height: scaledFrames[0].height,
                        defaultDelay: defaultDelay
                    })
                });
                
                if (!createRes.ok) {
                    throw new Error('Failed to create GIF');
                }
                
                // Upload frames sequentially (ESP32 SD card can't handle parallel writes)
                const totalFrames = scaledFrames.length;
                const MAX_RETRIES = 3;
                
                for (let i = 0; i < totalFrames; i++) {
                    const frame = scaledFrames[i];
                    const bmpData = createBmp(frame.imageData, frame.width, frame.height);
                    
                    let uploaded = false;
                    for (let attempt = 0; attempt < MAX_RETRIES && !uploaded; attempt++) {
                        try {
                            if (attempt > 0) {
                                updateProgress(`Retrying frame ${i + 1} (attempt ${attempt + 1})...`,
                                    10 + ((i + 1) / totalFrames) * 85);
                                await new Promise(r => setTimeout(r, 1000 * attempt));
                            }
                            
                            const formData = new FormData();
                            formData.append('frame', new Blob([bmpData], { type: 'image/bmp' }), `${i}.bmp`);
                            
                            const res = await fetch(`/api/gif/${gifName}/frame/${i}`, {
                                method: 'POST',
                                body: formData
                            });
                            
                            if (!res.ok) throw new Error(`HTTP ${res.status}`);
                            uploaded = true;
                        } catch (e) {
                            console.warn(`Frame ${i} attempt ${attempt + 1} failed:`, e.message);
                        }
                    }
                    
                    if (!uploaded) {
                        // Abort: tell ESP32 to reset upload state, delete partial GIF
                        await fetch('/api/abort-upload', { method: 'POST' }).catch(() => {});
                        await fetch(`/api/gif/${gifName}`, { method: 'DELETE' }).catch(() => {});
                        throw new Error(`Upload failed at frame ${i + 1}/${totalFrames} after ${MAX_RETRIES} retries`);
                    }
                    
                    updateProgress(`Uploading frames ${i + 1}/${totalFrames}...`, 10 + ((i + 1) / totalFrames) * 85);
                }
                
                // Upload original GIF file for preview
                updateProgress('Saving original...', 98);
                let origUploaded = false;
                for (let attempt = 0; attempt < MAX_RETRIES && !origUploaded; attempt++) {
                    try {
                        if (attempt > 0) await new Promise(r => setTimeout(r, 1000 * attempt));
                        const originalFormData = new FormData();
                        originalFormData.append('original', file, file.name);
                        const origRes = await fetch(`/api/gif/${gifName}/original`, {
                            method: 'POST',
                            body: originalFormData
                        });
                        if (!origRes.ok) throw new Error(`HTTP ${origRes.status}`);
                        origUploaded = true;
                    } catch (e) {
                        console.warn(`Original upload attempt ${attempt + 1} failed:`, e.message);
                    }
                }
                if (!origUploaded) {
                    // Frames uploaded fine but original failed ??not critical, just warn
                    console.warn('Could not upload original GIF file for preview');
                    await fetch('/api/abort-upload', { method: 'POST' }).catch(() => {});
                }
                
                updateProgress('Done!', 100);
                await loadGifs();
                hideProgress();
                showToast('GIF uploaded successfully');
                
            } catch (err) {
                console.error(err);
                hideProgress();
                showToast('Error: ' + err.message);
            }
        }
        
        // Scale frames to fit within maxSize while maintaining aspect ratio
        function scaleFrames(frames, maxSize) {
            if (frames.length === 0) return frames;
            
            const srcWidth = frames[0].width;
            const srcHeight = frames[0].height;
            
            // Check if scaling is needed
            if (srcWidth <= maxSize && srcHeight <= maxSize) {
                return frames;
            }
            
            // Calculate new dimensions
            let newWidth, newHeight;
            if (srcWidth > srcHeight) {
                newWidth = maxSize;
                newHeight = Math.round(srcHeight * maxSize / srcWidth);
            } else {
                newHeight = maxSize;
                newWidth = Math.round(srcWidth * maxSize / srcHeight);
            }
            
            // Create scaling canvas
            const scaleCanvas = document.createElement('canvas');
            scaleCanvas.width = newWidth;
            scaleCanvas.height = newHeight;
            const scaleCtx = scaleCanvas.getContext('2d');
            scaleCtx.imageSmoothingEnabled = true;
            scaleCtx.imageSmoothingQuality = 'high';
            
            // Temporary canvas for source image
            const srcCanvas = document.createElement('canvas');
            srcCanvas.width = srcWidth;
            srcCanvas.height = srcHeight;
            const srcCtx = srcCanvas.getContext('2d');
            
            return frames.map(frame => {
                // Put original frame data to source canvas
                srcCtx.putImageData(frame.imageData, 0, 0);
                
                // Scale to target canvas
                scaleCtx.clearRect(0, 0, newWidth, newHeight);
                scaleCtx.drawImage(srcCanvas, 0, 0, newWidth, newHeight);
                
                // Get scaled image data
                const scaledData = scaleCtx.getImageData(0, 0, newWidth, newHeight);
                
                return {
                    imageData: scaledData,
                    width: newWidth,
                    height: newHeight,
                    delay: frame.delay
                };
            });
        }
        
        // Parse GIF using browser's native decoding via ImageDecoder API or fallback
        async function parseGif(data) {
            // Try ImageDecoder API (Chrome/Edge) with proper frame composition
            if ('ImageDecoder' in window) {
                try {
                    const decoder = new ImageDecoder({
                        data: data.buffer,
                        type: 'image/gif'
                    });
                    
                    await decoder.tracks.ready;
                    const track = decoder.tracks.selectedTrack;
                    const frameCount = track.frameCount;
                    
                    const frames = [];
                    const canvas = document.createElement('canvas');
                    const ctx = canvas.getContext('2d', { willReadFrequently: true });
                    let initialized = false;
                    
                    for (let i = 0; i < frameCount; i++) {
                        const result = await decoder.decode({ frameIndex: i, completeFramesOnly: true });
                        const frame = result.image;
                        
                        if (!initialized) {
                            canvas.width = frame.codedWidth;
                            canvas.height = frame.codedHeight;
                            ctx.fillStyle = '#000';
                            ctx.fillRect(0, 0, canvas.width, canvas.height);
                            initialized = true;
                        }
                        
                        // Draw frame onto canvas
                        ctx.drawImage(frame, 0, 0);
                        
                        // Capture composited result
                        frames.push({
                            imageData: ctx.getImageData(0, 0, canvas.width, canvas.height),
                            width: canvas.width,
                            height: canvas.height,
                            delay: Math.max((frame.duration || 0) / 1000, 20) // duration is in microseconds
                        });
                        
                        frame.close();
                    }
                    
                    decoder.close();
                    if (frames.length > 0) return frames;
                } catch (e) {
                    console.log('ImageDecoder failed:', e);
                }
            }
            
            // Fallback: manual GIF parsing
            return parseGifManual(data);
        }
        
        // Manual GIF parser as fallback
        function parseGifManual(data) {
            const buf = data;
            let p = 0;
            
            function read(n) { const r = buf.subarray(p, p + n); p += n; return r; }
            function readByte() { return buf[p++]; }
            function readWord() { return buf[p++] | (buf[p++] << 8); }
            
            // Header
            const sig = String.fromCharCode(...read(6));
            if (!sig.startsWith('GIF')) throw new Error('Invalid GIF');
            
            // Logical Screen Descriptor
            const width = readWord();
            const height = readWord();
            const packed = readByte();
            const bgIndex = readByte();
            readByte(); // aspect ratio
            
            const gctFlag = (packed >> 7) & 1;
            const gctSize = 1 << ((packed & 7) + 1);
            
            let gct = null;
            if (gctFlag) {
                gct = [];
                for (let i = 0; i < gctSize; i++) {
                    gct.push([buf[p++], buf[p++], buf[p++]]);
                }
            }
            
            const frames = [];
            const canvas = document.createElement('canvas');
            canvas.width = width;
            canvas.height = height;
            const ctx = canvas.getContext('2d');
            ctx.fillStyle = '#000';
            ctx.fillRect(0, 0, width, height);
            
            let delay = 100, transIndex = -1, disposal = 0;
            
            while (p < buf.length) {
                const block = readByte();
                
                if (block === 0x21) { // Extension
                    const ext = readByte();
                    if (ext === 0xF9) { // GCE
                        const blockSize = readByte(); // size (always 4)
                        const packed2 = readByte();
                        disposal = (packed2 >> 2) & 7;
                        const hasTransparent = (packed2 & 1) !== 0;
                        delay = readWord() * 10;
                        if (delay < 20) delay = 100;
                        transIndex = hasTransparent ? readByte() : (readByte(), -1);
                        readByte(); // terminator
                    } else {
                        while (buf[p]) p += buf[p] + 1;
                        p++;
                    }
                } else if (block === 0x2C) { // Image
                    const x = readWord(), y = readWord();
                    const w = readWord(), h = readWord();
                    const imgPacked = readByte();
                    
                    const lctFlag = (imgPacked >> 7) & 1;
                    const interlaced = (imgPacked >> 6) & 1;
                    const lctSize = 1 << ((imgPacked & 7) + 1);
                    
                    let ct = gct;
                    if (lctFlag) {
                        ct = [];
                        for (let i = 0; i < lctSize; i++) {
                            ct.push([buf[p++], buf[p++], buf[p++]]);
                        }
                    }
                    
                    // LZW decode
                    const minCode = readByte();
                    let chunks = [];
                    while (buf[p]) {
                        const sz = buf[p++];
                        chunks.push(buf.subarray(p, p + sz));
                        p += sz;
                    }
                    p++; // terminator
                    
                    const pixels = lzwDecode(minCode, chunks, w * h);
                    
                    // Save for disposal 3
                    let prev = null;
                    if (disposal === 3) prev = ctx.getImageData(0, 0, width, height);
                    
                    // Draw
                    const imgData = ctx.getImageData(0, 0, width, height);
                    let pi = 0;
                    const passes = interlaced ? [[0,8],[4,8],[2,4],[1,2]] : [[0,1]];
                    for (const [start, step] of passes) {
                        for (let row = start; row < h; row += step) {
                            for (let col = 0; col < w; col++) {
                                const ci = pixels[pi++];
                                if (ci !== transIndex && ct && ct[ci]) {
                                    const px = x + col, py = y + row;
                                    if (px < width && py < height) {
                                        const off = (py * width + px) * 4;
                                        imgData.data[off] = ct[ci][0];
                                        imgData.data[off+1] = ct[ci][1];
                                        imgData.data[off+2] = ct[ci][2];
                                        imgData.data[off+3] = 255;
                                    }
                                }
                            }
                        }
                    }
                    ctx.putImageData(imgData, 0, 0);
                    
                    frames.push({
                        imageData: ctx.getImageData(0, 0, width, height),
                        width, height, delay
                    });
                    
                    // Disposal for next frame
                    if (disposal === 2) {
                        // Clear to background
                        ctx.fillStyle = '#000';
                        ctx.fillRect(x, y, w, h);
                    }
                    if (disposal === 3 && prev) ctx.putImageData(prev, 0, 0);
                    
                    transIndex = -1;
                    disposal = 0;
                } else if (block === 0x3B) {
                    break;
                } else if (block === 0) {
                    continue;
                } else {
                    break;
                }
            }
            return frames;
        }
        
        function lzwDecode(minCode, chunks, size) {
            let data = [];
            for (const c of chunks) for (let i = 0; i < c.length; i++) data.push(c[i]);
            
            const clear = 1 << minCode;
            const eoi = clear + 1;
            let codeSize = minCode + 1;
            let codeMask = (1 << codeSize) - 1;
            let next = eoi + 1;
            
            let dict = [];
            for (let i = 0; i < clear; i++) dict[i] = [i];
            
            const out = [];
            let bits = 0, bitCount = 0, bytePos = 0;
            let prev = -1;
            
            function readCode() {
                while (bitCount < codeSize && bytePos < data.length) {
                    bits |= data[bytePos++] << bitCount;
                    bitCount += 8;
                }
                const code = bits & codeMask;
                bits >>= codeSize;
                bitCount -= codeSize;
                return code;
            }
            
            while (out.length < size) {
                const code = readCode();
                if (code === eoi) break;
                if (code === clear) {
                    codeSize = minCode + 1;
                    codeMask = (1 << codeSize) - 1;
                    next = eoi + 1;
                    dict = [];
                    for (let i = 0; i < clear; i++) dict[i] = [i];
                    prev = -1;
                    continue;
                }
                
                let entry;
                if (code < next) {
                    entry = dict[code];
                } else if (code === next && prev >= 0) {
                    entry = [...dict[prev], dict[prev][0]];
                } else break;
                
                for (const px of entry) out.push(px);
                
                if (prev >= 0 && next < 4096) {
                    dict[next++] = [...dict[prev], entry[0]];
                    if (next > codeMask && codeSize < 12) {
                        codeSize++;
                        codeMask = (1 << codeSize) - 1;
                    }
                }
                prev = code;
            }
            return out;
        }
        
        // Create 16-bit RGB565 BMP from ImageData (smaller file size, faster upload)
        function createBmp(imageData, width, height) {
            // Use imageData's actual dimensions
            width = imageData.width;
            height = imageData.height;
            
            // 16-bit RGB565 format with BI_BITFIELDS compression
            const headerSize = 14 + 40 + 12; // BMP header + DIB header + RGB masks
            const rowSize = Math.floor((width * 2 + 3) / 4) * 4;  // 2 bytes per pixel, padded to 4 bytes
            const pixelDataSize = rowSize * height;
            const fileSize = headerSize + pixelDataSize;
            
            const buffer = new ArrayBuffer(fileSize);
            const view = new DataView(buffer);
            const data8 = new Uint8Array(buffer);
            
            // BMP Header (14 bytes)
            view.setUint8(0, 0x42); // 'B'
            view.setUint8(1, 0x4D); // 'M'
            view.setUint32(2, fileSize, true);
            view.setUint32(6, 0, true);           // reserved
            view.setUint32(10, headerSize, true); // pixel data offset
            
            // DIB Header (40 bytes) - BITMAPINFOHEADER
            view.setUint32(14, 40, true);         // header size
            view.setInt32(18, width, true);       // width
            view.setInt32(22, -height, true);     // height (negative = top-down)
            view.setUint16(26, 1, true);          // planes
            view.setUint16(28, 16, true);         // bits per pixel
            view.setUint32(30, 3, true);          // compression: BI_BITFIELDS
            view.setUint32(34, pixelDataSize, true);  // image size
            view.setUint32(38, 2835, true);       // X pixels per meter
            view.setUint32(42, 2835, true);       // Y pixels per meter
            view.setUint32(46, 0, true);          // colors in color table
            view.setUint32(50, 0, true);          // important colors
            
            // RGB565 color masks (12 bytes)
            view.setUint32(54, 0xF800, true);     // Red mask:   11111 000000 00000
            view.setUint32(58, 0x07E0, true);     // Green mask: 00000 111111 00000
            view.setUint32(62, 0x001F, true);     // Blue mask:  00000 000000 11111
            
            // Pixel data (RGB565 format, rows padded to 4 bytes)
            const padding = rowSize - width * 2;
            let offset = headerSize;
            
            for (let y = 0; y < height; y++) {
                for (let x = 0; x < width; x++) {
                    const srcIdx = (y * width + x) * 4;
                    const r = imageData.data[srcIdx];
                    const g = imageData.data[srcIdx + 1];
                    const b = imageData.data[srcIdx + 2];
                    // Convert to RGB565: RRRRR GGGGGG BBBBB
                    const rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
                    view.setUint16(offset, rgb565, true);
                    offset += 2;
                }
                // Add padding bytes
                for (let p = 0; p < padding; p++) {
                    data8[offset++] = 0;
                }
            }
            
            return buffer;
        }
        
        // Load GIF list
        async function loadGifs() {
            try {
                const res = await fetch('/api/gifs');
                gifs = await res.json();
                renderGifList();
            } catch (err) {
                console.error(err);
            }
        }
        
        function renderGifList() {
            gifCount.textContent = `${gifs.length} items`;
            
            if (gifs.length === 0) {
                gifList.innerHTML = `
                    <li class="empty-state">
                        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
                            <rect x="3" y="3" width="18" height="18" rx="2"/>
                            <circle cx="8.5" cy="8.5" r="1.5"/>
                            <path d="M21 15l-5-5L5 21"/>
                        </svg>
                        <p>No GIFs yet</p>
                    </li>
                `;
                return;
            }
            
            gifList.innerHTML = gifs.map((gif, index) => `
                <li class="gif-item" draggable="true" data-index="${index}" data-name="${gif.name}">
                    <div class="drag-handle">
                        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                            <line x1="8" y1="6" x2="16" y2="6"/>
                            <line x1="8" y1="12" x2="16" y2="12"/>
                            <line x1="8" y1="18" x2="16" y2="18"/>
                        </svg>
                    </div>
                    <div class="gif-preview">
                        <img src="/api/gif/${gif.name}/original" onerror="this.src='/api/gif/${gif.name}/frame/0'" alt="${gif.name}">
                    </div>
                    <div class="gif-info">
                        <div class="gif-name">${gif.name}</div>
                        <div class="gif-meta">${gif.frameCount} frames 繚 ${gif.width}?${gif.height}</div>
                    </div>
                    <div class="gif-actions">
                        <button class="btn btn-icon" onclick="showPreview('${gif.name}')" title="Preview">
                            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                                <path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"/>
                                <circle cx="12" cy="12" r="3"/>
                            </svg>
                        </button>
                        <button class="btn btn-icon btn-danger" onclick="showDeleteModal('${gif.name}')" title="Delete">
                            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                                <path d="M3 6h18"/>
                                <path d="M19 6v14a2 2 0 01-2 2H7a2 2 0 01-2-2V6"/>
                                <path d="M8 6V4a2 2 0 012-2h4a2 2 0 012 2v2"/>
                            </svg>
                        </button>
                    </div>
                </li>
            `).join('');
            
            // Setup drag and drop
            setupDragDrop();
        }
        
        function setupDragDrop() {
            const items = gifList.querySelectorAll('.gif-item');
            let draggedItem = null;
            
            items.forEach(item => {
                item.addEventListener('dragstart', (e) => {
                    draggedItem = item;
                    item.classList.add('dragging');
                    e.dataTransfer.effectAllowed = 'move';
                });
                
                item.addEventListener('dragend', () => {
                    item.classList.remove('dragging');
                    draggedItem = null;
                });
                
                item.addEventListener('dragover', (e) => {
                    e.preventDefault();
                    if (draggedItem && draggedItem !== item) {
                        const rect = item.getBoundingClientRect();
                        const midY = rect.top + rect.height / 2;
                        if (e.clientY < midY) {
                            item.parentNode.insertBefore(draggedItem, item);
                        } else {
                            item.parentNode.insertBefore(draggedItem, item.nextSibling);
                        }
                    }
                });
                
                item.addEventListener('drop', async (e) => {
                    e.preventDefault();
                    // Get new order
                    const newOrder = Array.from(gifList.querySelectorAll('.gif-item'))
                        .map(el => el.dataset.name);
                    
                    try {
                        await fetch('/api/reorder', {
                            method: 'POST',
                            headers: { 'Content-Type': 'application/json' },
                            body: JSON.stringify({ order: newOrder })
                        });
                        await loadGifs();
                    } catch (err) {
                        console.error(err);
                        showToast('Failed to reorder');
                    }
                });
            });
        }
        
        // Delete modal
        function showDeleteModal(name) {
            deleteTarget = name;
            document.getElementById('deleteGifName').textContent = name;
            document.getElementById('deleteModal').classList.add('active');
        }
        
        function closeDeleteModal() {
            document.getElementById('deleteModal').classList.remove('active');
            deleteTarget = null;
        }
        
        async function confirmDelete() {
            if (!deleteTarget) return;
            
            try {
                const res = await fetch(`/api/gif/${deleteTarget}`, { method: 'DELETE' });
                if (res.ok) {
                    await loadGifs();
                    showToast('GIF deleted');
                } else {
                    showToast('Failed to delete');
                }
            } catch (err) {
                showToast('Error: ' + err.message);
            }
            
            closeDeleteModal();
        }
        
        // Preview modal
        let previewFrames = [];
        let previewFrameIndex = 0;
        
        async function showPreview(name) {
            const gif = gifs.find(g => g.name === name);
            if (!gif) return;
            
            document.getElementById('previewTitle').textContent = name;
            document.getElementById('previewModal').classList.add('active');
            
            const canvas = document.getElementById('previewCanvas');
            const ctx = canvas.getContext('2d');
            
            // Try to load original GIF first
            try {
                const res = await fetch(`/api/gif/${name}/original`);
                if (res.ok) {
                    const blob = await res.blob();
                    const arrayBuffer = await blob.arrayBuffer();
                    const frames = await parseGif(new Uint8Array(arrayBuffer));
                    
                    if (frames.length > 0) {
                        canvas.width = frames[0].width;
                        canvas.height = frames[0].height;
                        
                        previewFrames = frames.map(f => ({
                            imageData: f.imageData,
                            delay: f.delay || 100
                        }));
                        
                        previewFrameIndex = 0;
                        function animateOriginal() {
                            if (!document.getElementById('previewModal').classList.contains('active')) {
                                return;
                            }
                            
                            const frame = previewFrames[previewFrameIndex];
                            ctx.putImageData(frame.imageData, 0, 0);
                            
                            previewFrameIndex = (previewFrameIndex + 1) % previewFrames.length;
                            previewInterval = setTimeout(animateOriginal, frame.delay);
                        }
                        animateOriginal();
                        return;
                    }
                }
            } catch (e) {
                console.log('Original GIF not available, falling back to frames');
            }
            
            // Fallback: load BMP frames
            canvas.width = gif.width;
            canvas.height = gif.height;
            
            previewFrames = [];
            const defaultDelay = gif.defaultDelay || 100;
            for (let i = 0; i < gif.frameCount; i++) {
                const img = new Image();
                img.src = `/api/gif/${name}/frame/${i}`;
                await new Promise(resolve => {
                    img.onload = resolve;
                    img.onerror = resolve;
                });
                previewFrames.push({ img, delay: defaultDelay });
            }
            
            // Animate
            previewFrameIndex = 0;
            function animate() {
                if (!document.getElementById('previewModal').classList.contains('active')) {
                    return;
                }
                
                const frame = previewFrames[previewFrameIndex];
                ctx.clearRect(0, 0, canvas.width, canvas.height);
                ctx.drawImage(frame.img, 0, 0);
                
                previewFrameIndex = (previewFrameIndex + 1) % previewFrames.length;
                previewInterval = setTimeout(animate, frame.delay);
            }
            
            animate();
        }
        
        function closePreviewModal() {
            document.getElementById('previewModal').classList.remove('active');
            if (previewInterval) {
                clearTimeout(previewInterval);
                previewInterval = null;
            }
        }
        
        // Progress
        function showProgress(text) {
            progressText.textContent = text;
            progressFill.style.width = '0%';
            progressOverlay.classList.add('active');
        }
        
        function updateProgress(text, percent) {
            progressText.textContent = text;
            progressFill.style.width = percent + '%';
        }
        
        function hideProgress() {
            progressOverlay.classList.remove('active');
        }
        
        // Toast
        function showToast(message) {
            const toast = document.getElementById('toast');
            toast.textContent = message;
            toast.classList.add('show');
            setTimeout(() => toast.classList.remove('show'), 3000);
        }
        
        // Close modals on backdrop click
        document.querySelectorAll('.modal').forEach(modal => {
            modal.addEventListener('click', (e) => {
                if (e.target === modal) {
                    modal.classList.remove('active');
                    if (previewInterval) {
                        clearTimeout(previewInterval);
                        previewInterval = null;
                    }
                }
            });
        });
        
        // App mode
        async function loadAppMode() {
            try {
                const res = await fetch('/api/mode');
                const data = await res.json();
                const container = document.getElementById('appButtons');
                const label = document.getElementById('currentApp');
                label.textContent = data.apps[data.current] || '--';
                container.innerHTML = '';
                data.apps.forEach((name, i) => {
                    const btn = document.createElement('button');
                    btn.className = 'btn ' + (i === data.current ? 'btn-primary' : 'btn-secondary');
                    btn.textContent = name;
                    btn.onclick = async () => {
                        await fetch('/api/mode', {
                            method: 'POST',
                            headers: {'Content-Type': 'application/json'},
                            body: JSON.stringify({app: i})
                        });
                        await loadAppMode();
                    };
                    container.appendChild(btn);
                });
            } catch(e) { console.error('loadAppMode', e); }
        }

        // Initial load
        loadAppMode();
        loadGifs();
    </script>
</body>
</html>
)rawliteral";

// WiFi configuration page HTML
const char WIFI_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-TW">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Holocubic - WiFi Setup</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        :root {
            --bg: #f5f5f7; --card: #ffffff; --text: #1d1d1f;
            --sub: #86868b; --accent: #0071e3; --border: #d2d2d7;
            --danger: #ff3b30; --success: #34c759; --radius: 12px;
        }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: var(--bg); color: var(--text); min-height: 100vh;
            display: flex; align-items: center; justify-content: center;
            padding: 24px;
        }
        .card {
            background: var(--card); border-radius: var(--radius);
            box-shadow: 0 2px 12px rgba(0,0,0,0.08);
            width: 100%; max-width: 400px; padding: 32px;
        }
        h1 { font-size: 24px; font-weight: 600; margin-bottom: 4px; }
        .subtitle { color: var(--sub); font-size: 14px; margin-bottom: 24px; }
        .status {
            display: flex; align-items: center; gap: 8px;
            padding: 12px 16px; border-radius: 8px;
            background: var(--bg); margin-bottom: 24px; font-size: 14px;
        }
        .dot {
            width: 8px; height: 8px; border-radius: 50%;
            flex-shrink: 0;
        }
        .dot.green { background: var(--success); }
        .dot.orange { background: #ff9500; }
        .field { margin-bottom: 16px; }
        label {
            display: block; font-size: 13px; font-weight: 500;
            color: var(--sub); margin-bottom: 6px;
        }
        input[type="text"], input[type="password"] {
            width: 100%; padding: 10px 14px; border: 1px solid var(--border);
            border-radius: 8px; font-size: 15px; outline: none;
            transition: border-color 0.2s;
        }
        input:focus { border-color: var(--accent); }
        .btn {
            width: 100%; padding: 12px; border: none; border-radius: 8px;
            font-size: 15px; font-weight: 500; cursor: pointer;
            transition: opacity 0.2s;
        }
        .btn:hover { opacity: 0.85; }
        .btn-primary { background: var(--accent); color: white; }
        .btn:disabled { opacity: 0.5; cursor: not-allowed; }
        .msg {
            margin-top: 16px; padding: 10px 14px; border-radius: 8px;
            font-size: 14px; display: none;
        }
        .msg.error { display: block; background: #fff0f0; color: var(--danger); }
        .msg.success { display: block; background: #f0fff4; color: var(--success); }
        .back {
            display: block; text-align: center; margin-top: 16px;
            color: var(--accent); text-decoration: none; font-size: 14px;
        }
    </style>
</head>
<body>
    <div class="card">
        <h1>WiFi Setup</h1>
        <p class="subtitle">Connect Holocubic to your network</p>

        <div class="status" id="status">
            <span class="dot" id="dot"></span>
            <span id="statusText">Loading...</span>
        </div>

        <form id="wifiForm">
            <div class="field">
                <label for="ssid">WiFi SSID</label>
                <input type="text" id="ssid" name="ssid" placeholder="Enter network name" required autocomplete="off">
            </div>
            <div class="field">
                <label for="password">Password</label>
                <input type="password" id="password" name="password" placeholder="Enter password" autocomplete="off">
            </div>
            <button type="submit" class="btn btn-primary" id="saveBtn">Save & Reboot</button>
        </form>

        <div class="msg" id="msg"></div>
        <a href="/" class="back">??Back to GIF Manager</a>
    </div>

    <script>
        async function loadStatus() {
            try {
                const res = await fetch('/api/wifi');
                const data = await res.json();
                const dot = document.getElementById('dot');
                const st = document.getElementById('statusText');
                if (data.mode === 'STA') {
                    dot.className = 'dot green';
                    st.textContent = 'Connected to "' + (data.ssid || '?') + '" (' + data.ip + ')';
                } else {
                    dot.className = 'dot orange';
                    st.textContent = 'AP Mode (' + data.ip + ') ??not connected';
                }
                if (data.ssid) document.getElementById('ssid').value = data.ssid;
            } catch (e) {
                document.getElementById('statusText').textContent = 'Cannot load status';
            }
        }

        document.getElementById('wifiForm').addEventListener('submit', async (e) => {
            e.preventDefault();
            const btn = document.getElementById('saveBtn');
            const msg = document.getElementById('msg');
            const ssid = document.getElementById('ssid').value.trim();
            const password = document.getElementById('password').value;

            if (!ssid) { msg.className = 'msg error'; msg.textContent = 'SSID is required'; return; }

            btn.disabled = true;
            btn.textContent = 'Saving...';
            msg.className = 'msg'; msg.style.display = 'none';

            try {
                const res = await fetch('/api/wifi', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ ssid, password })
                });
                const data = await res.json();
                if (res.ok) {
                    msg.className = 'msg success';
                    msg.textContent = 'Saved! Rebooting device... Please reconnect to your network.';
                } else {
                    throw new Error(data.error || 'Save failed');
                }
            } catch (err) {
                msg.className = 'msg error';
                msg.textContent = err.message;
                btn.disabled = false;
                btn.textContent = 'Save & Reboot';
            }
        });

        loadStatus();
    </script>
</body>
</html>
)rawliteral";

#endif // WEB_HTML_H
