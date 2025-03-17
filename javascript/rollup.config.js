import terser from '@rollup/plugin-terser';

console.log(terser);

export default {
  input: "src/storyletFramework.js", // Aggregation file as the entry point
  output: [
    {
      file: "dist/storyletFramework.js",
      format: "esm", // ES Module format
      sourcemap: true,
    },
    {
      file: "dist/storyletFramework.min.js",
      format: "iife", 
      name: "StoryletFramework", 
      plugins: [terser()],
      sourcemap: true,
    },
  ],
};