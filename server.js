const express = require('express');
const rateLimiter = require('./build/Release/rate_limiter.node');

const app = express();
const PORT = 3000;

try {
	rateLimiter.initSharedMemory({ maxTokens: 100, windowMs: 1000 });
	console.log(" C++ Shared Memory Initialized");
} catch (error) {
	console.error(" Failed to initialize:", error);
	process.exit(1);
}

app.use((req, res, next) => {
	const clientIp = req.headers['x-forwarded-for'] || req.socket.remoteAddress || '127.0.0.1';
	if (!rateLimiter.consumeTokenFast(clientIp)) {
		return res.status(429).json({ error: "Too Many Requests" });
	}
	next();
});

app.get('/', (req, res) => res.json({ status: "success" }));

app.listen(PORT, () => {
	console.log(' Server running on http://localhost:${PORT}');
	console.log(' Native Rate Limiter ACTIVE.');
});
