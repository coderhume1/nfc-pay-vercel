import { prisma } from "@/lib/prisma";

export const dynamic = "force-dynamic";
export const runtime = "nodejs";

export default async function Page({ params }: { params: { terminalId: string } }) {
  const { terminalId } = params;
  const session = await prisma.session.findFirst({
    where: { terminalId, status: "pending" },
    orderBy: { createdAt: "desc" },
  });
  return (
    <main>
      <h1>Checkout – {terminalId}</h1>
      {session ? (
        <>
          <p><b>Session:</b> {session.id}</p>
          <p><b>Amount:</b> {session.amount} {session.currency}</p>
          <p><b>Status:</b> <span id="status">{session.status}</span></p>
          <form method="post" action="/api/sandbox/pay">
            <input type="hidden" name="sessionId" value={session.id} />
            <button type="submit">Approve (Sandbox)</button>
          </form>
        </>
      ) : (
        <p>No pending session for this terminal.</p>
      )}
    </main>
  );
}
