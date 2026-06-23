export default {
  fetch(request) {
    const url = new URL(request.url);

    if (url.pathname === "/api/download") {
      return Response.redirect(
        new URL(`/downloads/Pilgrim-of-the-Thorn-macOS-v${__PILGRIM_VERSION__}.dmg`, url.origin).toString(),
        302
      );
    }

    return new Response("Not found", {
      status: 404,
      headers: {
        "content-type": "text/plain; charset=utf-8"
      }
    });
  }
} satisfies ExportedHandler;
