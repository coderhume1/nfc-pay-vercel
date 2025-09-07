import { prisma } from "@/lib/prisma";
import { isAdminAuthed } from "@/lib/auth";

export const dynamic = "force-dynamic";
export const runtime = "nodejs";

export default async function DevicesPage() {
  if (!isAdminAuthed()) {
    return <main><h1>Unauthorized</h1></main>;
  }
  const devices = await prisma.device.findMany({ orderBy: { createdAt: "desc" }, take: 100 });
  return (
    <main>
      <h1>Devices</h1>
      <form method="post" action="/api/admin/devices/upsert" style={{marginBottom: 16}}>
        <input name="deviceId" placeholder="deviceId (hex)" required />
        <input name="storeCode" placeholder="STORE01" />
        <input name="terminalId" placeholder="(auto if empty)" />
        <input name="amount" placeholder="0" />
        <input name="currency" placeholder="USD" />
        <button type="submit">Upsert</button>
      </form>
      <table border={1} cellPadding={6}>
        <thead><tr><th>deviceId</th><th>store</th><th>terminal</th><th>amount</th><th>ccy</th><th>status</th><th>created</th><th>action</th></tr></thead>
        <tbody>
          {devices.map(d => (
            <tr key={d.deviceId}>
              <td>{d.deviceId}</td><td>{d.storeCode}</td><td>{d.terminalId}</td><td>{d.amount}</td><td>{d.currency}</td><td>{d.status}</td><td>{d.createdAt.toISOString()}</td>
              <td>
                <form method="post" action="/api/admin/devices/delete">
                  <input type="hidden" name="deviceId" value={d.deviceId} />
                  <button>Delete</button>
                </form>
              </td>
            </tr>
          ))}
        </tbody>
      </table>
    </main>
  );
}
