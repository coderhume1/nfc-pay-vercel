# NFC Pay Starter (Vercel)

**Deploy in minutes on Vercel + Neon.**

## 1) Neon
- Create a project; copy the **pooled** connection URL (has `-pooler`) and append `?sslmode=require`.

## 2) Vercel
- Create a new project from this repo.
- Set environment variables (Project Settings → Environment Variables):
  - `DATABASE_URL=postgresql://USER:PASS@ep-xxxx-pooler.us-east-1.aws.neon.tech/neondb?sslmode=require`
  - `API_KEY=esp32_test_api_key_123456`
  - `ADMIN_KEY=admin_demo_key_123456`
  - `PUBLIC_BASE_URL=https://<project>.vercel.app`
  - `DEFAULT_STORE_CODE=STORE01`
  - `DEFAULT_AMOUNT=0`
  - `DEFAULT_CURRENCY=USD`
  - `TERMINAL_PREFIX=`
  - `TERMINAL_PAD=4`
- Build command is overridden in `vercel.json` to run Prisma migrations before Next build.

## 3) Test
- Visit `/admin` → login with `ADMIN_KEY`.
- Visit `/p/ADMIN_TEST` → use Approve (Sandbox) button.
- Device bootstrap: call `/api/v1/bootstrap` with headers `X-API-Key`, `X-Device-Id`.

## 4) ESP32
- Point firmware to `BASE_URL=https://<project>.vercel.app` and use the same `API_KEY`.

## Notes
- Schema lives in `prisma/schema.prisma`; migration in `prisma/migrations/0001_init`.
- If migrations fail at build, check `DATABASE_URL` is pooled + SSL. You can also run migrations once locally:
  - `npm i && cp .env.example .env` (fill DB) → `npx prisma migrate deploy`
