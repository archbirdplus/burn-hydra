const timer_opts = [
    ["initializing variables", 0.3, 0.4, "green"],
    ["waiting to send left", 0.0, 0.15, "orange"],
    ["waiting to recv left", 0.0, 0.15, "red"],
    ["grinding chain", 0.15, 0.7, "gray"],
    ["grinding basecase", 0.7, 0.15, "blue"],
    ["gather communication", 0.3, 0.4, "green"],
    ["waiting to send right", 0.85, 0.15, "purple"],
    ["waiting to recv right", 0.85, 0.15, "blue"],
]

const canvas = document.getElementById("canvas");
const ctx = canvas.getContext("2d");

const ranks = JSON.parse(data);
const rank_count = Object.keys(ranks).length;

function max_time() {
    const xs = ranks["rank 0"]['actively'];
    return xs[xs.length-1][1];
}

const max = max_time();

canvas.height = 400*max/100;
canvas.width = 600;
document.body.style.backgroundColor = 'black';

function render() {
    console.log("render...");
    const width = canvas.width / (rank_count * 1.1);
    const margin = width*0.1;
    const heightscale = canvas.height / max;
    for(var r = 0; r < rank_count; r++) {
        const timers = ranks["rank "+r];
        const x_pos = (rank_count - r - 1) * (width + margin)
        const timer_names = Object.keys(timers);
        for (opts of timer_opts) {
            const color = opts[3];
            ctx.fillStyle = color;
            if (!timers[opts[0]]) { continue; }
            for (start_stop of timers[opts[0]]) {
                const d = start_stop[1] - start_stop[0]
                ctx.fillRect(x_pos + width*opts[1], start_stop[0]*heightscale, width*opts[2], d*heightscale);
            }
        }
    }
    console.log("render done");
}
render();

