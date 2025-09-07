# NFC Pay – Auto-write to tag (ESP32)

This overlay adds **auto-write**: when you click **Generate Payment** in the dashboard (POST `/api/v1/payments/generate`), the server creates a TagWrite job for your ESP32 writer. The ESP32 polls `/api/v1/device/next`, writes the NDEF URI to the tag via PN532, then acks via `/api/v1/device/ack`.

## New ENV
```
BASE_URL=https://your-app.vercel.app
DEVICE_KEY=dev-device-key
DATABASE_URL=postgres://...
```

## Setup

1. **Bootstrap device** (once): ESP32 calls `GET /api/v1/bootstrap` with `X-Device-Key: DEVICE_KEY` header.
2. **Generate payment**: your UI should POST `/api/v1/payments/generate` with `{ amount, currency?, label?, autoWrite?: true }`.
3. **ESP32 poll**: device hits `/api/v1/device/next` and receives `{ job: { id, type: "NDEF_URI", payload: { uri } }}`.
4. **Write + ack**: device writes the tag then POST `/api/v1/device/ack` with `{ jobId, ok }`.

## Prisma
```
npx prisma generate
npx prisma migrate dev --name autowrite
# optional seed
npx ts-node prisma/seed.ts
```

## Firmware
See `firmware/device-writer` for a ready PlatformIO project for ESP32 + PN532 (I2C). Set `WIFI_SSID`, `WIFI_PASS`, `BASE_URL`, and `DEVICE_KEY` in `config.h`.
