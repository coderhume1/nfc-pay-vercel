// prisma/seed.ts
import { PrismaClient } from "@prisma/client";

const prisma = new PrismaClient();

async function main() {
  const key = process.env.DEVICE_KEY || "dev-device-key";
  const name = process.env.DEVICE_NAME || "ESP32 Writer #1";
  await prisma.device.upsert({
    where: { key },
    create: { key, name },
    update: { name },
  });
  console.log("Seeded device:", name);
}

main().finally(() => prisma.$disconnect());
