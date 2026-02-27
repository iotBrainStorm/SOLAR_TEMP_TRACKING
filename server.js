const http = require('http');
const fs = require('fs');
const path = require('path');

const PORT = 8000;
const DATA_DIR = path.join(__dirname, 'data');

const server = http.createServer((req, res) => {
  // Default to index.html (or monitor.html)
  let filePath = req.url === '/' ? path.join(DATA_DIR, 'monitor.html') : path.join(DATA_DIR, req.url);
  
  // Security: prevent path traversal
  const realPath = path.resolve(filePath);
  if (!realPath.startsWith(DATA_DIR)) {
    res.writeHead(403, { 'Content-Type': 'text/plain' });
    res.end('Forbidden');
    return;
  }

  // Read and serve file
  fs.readFile(filePath, (err, content) => {
    if (err) {
      if (err.code === 'ENOENT') {
        res.writeHead(404, { 'Content-Type': 'text/plain' });
        res.end('File not found');
      } else {
        res.writeHead(500, { 'Content-Type': 'text/plain' });
        res.end('Server error');
      }
    } else {
      // Determine content type
      const ext = path.extname(filePath).toLowerCase();
      let contentType = 'text/plain';
      if (ext === '.html') contentType = 'text/html';
      else if (ext === '.css') contentType = 'text/css';
      else if (ext === '.js') contentType = 'application/javascript';
      else if (ext === '.json') contentType = 'application/json';
      else if (ext === '.svg') contentType = 'image/svg+xml';

      res.writeHead(200, { 'Content-Type': contentType });
      res.end(content);
    }
  });
});

server.listen(PORT, () => {
  console.log(`Server running at http://localhost:${PORT}/`);
  console.log(`Monitor dashboard: http://localhost:${PORT}/monitor.html`);
  console.log(`Config page: http://localhost:${PORT}/config.html`);
  console.log(`Press Ctrl+C to stop`);
});
