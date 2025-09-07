export const metadata = { title: "NFC Pay Starter (Vercel)", description: "Next.js + Prisma + Neon" };
export default function RootLayout({ children }: { children: React.ReactNode }) {
  return (
    <html lang="en">
      <body style={{ fontFamily: "system-ui, Segoe UI, Roboto, Arial", margin: 20 }}>{children}</body>
    </html>
  );
}
