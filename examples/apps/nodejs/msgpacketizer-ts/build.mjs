import * as esbuild from "esbuild";
import { glob } from "glob";

// should glob manually to get all ts files in src directory
const entryPoints = glob.sync("./src/*.ts");

// Create a context for incremental builds
const context = await esbuild.context({
  // entrypoint file(s)
  entryPoints,
  // bundle modules together
  bundle: true,
  // 'node' 'browser' 'neutral'
  platform: "node",
  // // set node version if you need
  // target: ["node18.18"],
  // reflect the structure of outbase to outdir
  outbase: "./src",
  // output directory
  outdir: "./dist",
  // exclude modules from bundling
  external: [],
});

// Manually do an incremental build
const result = await context.rebuild();

// get cli option (--watch)
const watch = process.argv.includes("--watch");
if (watch) {
  // Enable watch mode
  await context.watch();
} else {
  // Dispose of the context
  context.dispose();
}
