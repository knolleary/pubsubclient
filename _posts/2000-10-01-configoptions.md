---
tag: docs
type: other
title: Configuration Options
---

The following configuration options can be used to configure the library.
They are contained in `PubSubClient.h`.

<dl>
    <dt><code>MQTT_MAX_PACKET_SIZE</code></dt>
    <dd>Sets the largest packet size, in bytes, the client will handle. Any
        packet received that exceeds this size will be ignored.
        <p>This value can be overridden by calling <a href="#setBufferSize"><code>setBufferSize(size)</code></a>.</p>
        <p>Default: 128 bytes</p>
    </dd>
    <dt><code>MQTT_KEEPALIVE</code></dt>
    <dd>Sets the keepalive interval, in seconds, the client will use. This
        is used to maintain the connection when no other packets are being
        sent or received.
        <p>This value can be overridden by calling <a href="#setKeepAlive"><code>setKeepAlive(keepAlive)</code></a>.</p>
        <p>Default: 15 seconds</p>
    </dd>
    <dt><code>MQTT_VERSION</code></dt>
    <dd>Sets the version of the MQTT protocol to use.
        <p>Default: MQTT 3.1.1</p>
    </dd>
    <dt><code>MQTT_MAX_TRANSFER_SIZE</code></dt>
    <dd>Sets the maximum number of bytes passed to the network client in each
        write call. Some hardware has a limit to how much data can be passed
        to them in one go, such as the Arduino Wifi Shield.
        <p>Default: undefined (complete packet passed in each write call)</p>
    </dd>
    <dt><code>MQTT_SOCKET_TIMEOUT</code></dt>
    <dd>Sets the timeout when reading from the network. This also applies as
        the timeout for calls to <code>connect</code>.
        <p>This value can be overridden by calling <a href="#setSocketTimeout"><code>setSocketTimeout(timeout)</code></a>.</p>
        <p>Default: 15 seconds</p>
    </dd>
</dl>