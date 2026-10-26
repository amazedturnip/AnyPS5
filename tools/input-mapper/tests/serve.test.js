import test from 'node:test';
import assert from 'node:assert/strict';
import {spawn} from 'node:child_process';
import {createServer, get} from 'node:http';
import {connect} from 'node:net';
import {once} from 'node:events';
import {fileURLToPath} from 'node:url';

const script = fileURLToPath(new URL('../serve.js', import.meta.url));

async function freePort() {
    const server = createServer();
    server.listen(0, '127.0.0.1');
    await once(server, 'listening');
    const {port} = server.address();
    server.close();
    await once(server, 'close');
    return port;
}

function rawRequest(port, target) {
    return new Promise((resolve, reject) => {
        const socket = connect(port, '127.0.0.1', () => socket.write(`GET ${target} HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n`));
        const chunks = [];
        socket.on('data', chunk => chunks.push(chunk));
        socket.on('error', reject);
        socket.on('close', () => resolve(Buffer.concat(chunks).toString('latin1')));
    });
}

function status(port, path) {
    return new Promise((resolve, reject) => {
        get({host: '127.0.0.1', port, path}, response => {
            response.resume();
            response.on('end', () => resolve(response.statusCode));
        }).on('error', reject);
    });
}

test('a request target that is not a valid URL gets 400 and the server keeps serving', async t => {
    const port = await freePort();
    const child = spawn(process.execPath, [script], {env: {...process.env, PORT: String(port)}, stdio: ['ignore', 'pipe', 'inherit']});
    t.after(() => child.kill());
    await once(child.stdout, 'data');
    assert.match(await rawRequest(port, '//'), /^HTTP\/1\.1 400 /u);
    assert.equal(child.exitCode, null);
    assert.equal(await status(port, '/'), 200);
});
