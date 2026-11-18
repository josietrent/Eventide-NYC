const bytes = Deno.readFileSync("C:/Users/josie/Documents/GTA Modding/GTA III/gta3.exe");

const start = 0x4a0fe0 - 0x400000;
const slice = bytes.slice(start, start + 32);
console.log("0x4a0fe0: " + Array.from(slice).map(b => b.toString(16).padStart(2, "0")).join(" "));
