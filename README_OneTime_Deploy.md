# One-Time Deployment (Minimal)

This repo contains a demo NFC checkout with an ESP32 NFC writer.

## 1) Cloud App (Vercel)
1. Fork or import this repo into your GitHub.
2. Create a new Vercel project from the repo.
3. In Vercel → Settings → Environment Variables, add:
   - `DATABASE_URL` — Postgres connection string
   - `API_KEY` — any random secret (used by device)
   - `ADMIN_KEY` — set to a password you will use to log in as admin
   - `PUBLIC_BASE_URL` — your site URL (e.g. `https://your-project.vercel.app`)
   - `DEFAULT_STORE_CODE` — e.g. `STORE01`
   - `DEFAULT_AMOUNT` — e.g. `0`
   - `DEFAULT_CURRENCY` — e.g. `USD`
   - `TERMINAL_PREFIX` — e.g. `STORE01-`
   - `TERMINAL_PAD` — e.g. `5`
4. Trigger a deploy. On first deploy, run Prisma migrations (Vercel will run them automatically if `DATABASE_URL` is set).

## 2) Admin Setup
1. Open `/admin`, click **Admin Login**, enter your `ADMIN_KEY` (it sets a cookie).
2. Go to **Admin → Devices**:
   - Add your ESP32 MAC as `deviceId` (uppercase, e.g. `08:B6:1F:AA:BB:CC`).
   - Choose a `storeCode` and **Save**. A terminal ID is assigned automatically.

## 3) ESP32
1. Open `firmware/esp32_nfc_softap_pay/esp32_nfc_softap_pay.ino` in Arduino IDE.
2. Install libraries (**Adafruit NeoPixel**, PN532) if prompted.
3. Set:
   - `BASE_URL` = your `PUBLIC_BASE_URL`
   - `API_KEY` = same as Vercel `API_KEY`
4. Flash the ESP32 and connect it to Wi‑Fi using the SoftAP portal.
5. On boot it writes the NFC tag for the terminal page (`/p/{terminalId}`) and waits.

## 4) Taking a Payment (Demo)
1. Open `/p/{terminalId}`.
2. Click **Generate Payment** (optionally set amount).
3. Tap the NFC tag or scan the URL and click **Approve (Sandbox)**.
4. The ESP32 shows **pending** → **paid** with LED + buzzer.

> Note: Only the newest pending can be approved; older pendings are automatically canceled.
