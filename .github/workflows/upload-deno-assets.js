const fs = require("fs").promises;

const compiled_extensions = [
  {
    path: "sqlite-vss-macos_x86_64/vector0.dylib",
    name: "deno-darwin-x86_64.vector0.dylib",
  },
  {
    path: "sqlite-vss-macos_x86_64/vss0.dylib",
    name: "deno-darwin-x86_64.vss0.dylib",
  },
  {
    path: "sqlite-vss-macos_aarch64/vector0.dylib",
    name: "deno-darwin-aarch64.vector0.dylib",
  },
  {
    path: "sqlite-vss-aarch64/vss0.dylib",
    name: "deno-darwin-aarch64.vss0.dylib",
  },
  {
    path: "sqlite-vss-linux_x86_64/vector0.so",
    name: "deno-linux-x86_64.vector0.so",
  },
  {
    path: "sqlite-vss-linux_x86_64/vss0.so",
    name: "deno-linux-linux-x86_64.vss0.so",
  },
];

module.exports = async ({ github, context }) => {
  const { owner, repo } = context.repo;
  const release = await github.rest.repos.getReleaseByTag({
    owner,
    repo,
    tag: process.env.GITHUB_REF.replace("refs/tags/", ""),
  });
  const release_id = release.data.id;

  await Promise.all(
    compiled_extensions.map(async ({ name, path }) => {
      return github.rest.repos.uploadReleaseAsset({
        owner,
        repo,
        release_id,
        name,
        data: await fs.readFile(path),
      });
    })
  );
};
