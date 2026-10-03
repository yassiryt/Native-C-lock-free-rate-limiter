# Native C++ Rate Limiter for Node.js

A high-performance rate limiter native addon that bypasses JavaScript garbage collection using OS shared memory and lock-free C++ concurrency.

## Features

- **Zero heap allocation**: Uses `mmap` for direct OS RAM mapping, bypassing V8 GC
- **Cache-aligned**: 64-byte padding prevents false-sharing across PM2 worker threads  
- **Lock-free**: Compare-And-Swap (CAS) operations at CPU level, no mutexes
- **Multi-process**: POSIX `shm_open`/`mmap` shares state across cluster processes
- **DoS-resistant hash**: FNV-1a seeded hash makes collision attacks impossible

## Build

```bash
npm install
npx node-gyp configure build
```

## Usage

```js
const rateLimiter = require('./build/Release/rate_limiter.node');

rateLimiter.initSharedMemory({ maxTokens: 100, windowMs: 1000 });

app.use((req, res, next) => {
  const ip = req.headers['x-forwarded-for'] || req.socket.remoteAddress;
  if (!rateLimiter.consumeTokenFast(ip)) {
    return res.status(429).json({ error: 'Too Many Requests' });
  }
  next();
});
```