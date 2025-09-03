const STREAM_URL = "https://corrections-chevy-hull-sol.trycloudflare.com/stream.m3u8";

export function setupVideoStream(authToken) {
    const video = document.getElementById("tankStream");
    
    if (Hls.isSupported()) {
        const hls = new Hls({
            xhrSetup: function (xhr, url) {
                xhr.setRequestHeader('Authorization', 'Bearer ' + authToken);
            }
        });
        hls.loadSource(STREAM_URL);
        hls.attachMedia(video);
        hls.on(Hls.Events.ERROR, function (event, data) {
            console.warn("HLS.js error:", data);
            if (data.fatal) {
                console.log("🔄 Reloading HLS stream...");
                hls.startLoad();
            }
        });
    } else if (video.canPlayType("application/vnd.apple.mpegurl")) {
        // Safari native HLS support (no way to set headers)
        video.src = STREAM_URL;
        video.addEventListener("error", () => {
            console.log("🔄 Reloading stream for Safari...");
            video.load();
        });
    } else {
        video.outerHTML = "<p>⚠️ Your browser does not support HLS streaming.</p>";
    }
}
