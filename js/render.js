const timer_opts = [
    ["actively", 0.7, 0.1, "darkgreen"],
    ["initializing variables", 0.3, 0.4, "green"],
    ["waiting to send left", 0.0, 0.15, "orange"],
    ["waiting to recv left", 0.0, 0.15, "red"],
    ["grinding chain", 0.15, 0.7, "gray"],
    ["grinding basecase", 0.7, 0.15, "blue"],
    ["gather communication", 0.3, 0.4, "green"],
    ["waiting to send right", 0.85, 0.15, "purple"],
    ["waiting to recv right", 0.85, 0.15, "blue"],
]
const text_color = "gray"
const text_height = 10
const graph_height = 30

const canvas = document.getElementById("canvas");
const ctx = canvas.getContext("2d");

const ranks = JSON.parse(data);
// TODO: handle nested metrics
const count = Object.keys(ranks).length;

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
    const width = canvas.width / (count * 1.1);
    const margin = width*0.1;
    const heightscale = canvas.height / max;
    var i = 0;
    for(r in ranks) {
        const timers = ranks[r];
        const x_pos = (count - i - 1) * (width + margin)
        const timer_names = Object.keys(timers);
        ctx.fillStyle = text_color
        ctx.fillText(r, x_pos + margin, text_height, width);
        for (opts of timer_opts) {
            const color = opts[3];
            ctx.fillStyle = color;
            if (!timers[opts[0]]) { continue; }
            for (start_stop of timers[opts[0]]) {
                const d = start_stop[1] - start_stop[0]
                ctx.fillRect(x_pos + width*opts[1], graph_height + start_stop[0]*heightscale, width*opts[2], d*heightscale);
            }
        }
        i += 1;
    }
    console.log("render done");
}
render();

