import Link from "next/link";
import { prisma } from "@/lib/prisma";
import { isAdminAuthed } from "@/lib/auth";

export const dynamic = "force-dynamic";
export const runtime = "nodejs";

export default async function Admin() {
  if (!isAdminAuthed()) {
    return (
      <main>
        <h1>Admin Login</h1>
        <form method="post" action="/api/admin/login">
          <input name="key" placeholder="Admin key" />
          <button type="submit">Login</button>
        </form>
      </main>
    );
  }
  const sessions = await prisma.session.findMany({ orderBy: { createdAt: "desc" }, take: 20 });
  return (
    <main>
      <h1>Admin</h1>
      <p><Link href="/admin/devices">Devices</Link></p>
      <h2>Recent Sessions</h2>
      <table border={1} cellPadding={6}>
        <thead><tr><th>id</th><th>terminal</th><th>amt</th><th>ccy</th><th>status</th><th>created</th></tr></thead>
        <tbody>
          {sessions.map(s => (
            <tr key={s.id}>
              <td>{s.id}</td><td>{s.terminalId}</td><td>{s.amount}</td><td>{s.currency}</td><td>{s.status}</td><td>{s.createdAt.toISOString()}</td>
            </tr>
          ))}
        </tbody>
      </table>
    </main>
  );
}
