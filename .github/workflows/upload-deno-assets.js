const fs = require("fs").promises;
const { createHash } = require("node:crypto");

module.exports = async ({ github, context }) => {
  const VERSION = process.env.GITHUB_REF_NAME;
  const { owner, repo } = context.repo;

  const compiled_extensions = [
    {
      path: `${process.env["ARTIFACT-MACOS-X86_64-EXTENSION"]}/vector0.dylib`,
      name: `sqlite-vss-${VERSION}-deno-darwin-x86_64.vector0.dylib`,
    },
    {
      path: `${process.env["ARTIFACT-MACOS-X86_64-EXTENSION"]}/vss0.dylib`,
      name: `sqlite-vss-${VERSION}-deno-darwin-x86_64.vss0.dylib`,
    },
    {
      path: `${process.env["ARTIFACT-MACOS-AARCH64-EXTENSION"]}/vector0.dylib`,
      name: `sqlite-vss-${VERSION}-deno-darwin-aarch64.vector0.dylib`,
    },
    {
      path: `${process.env["ARTIFACT-MACOS-AARCH64-EXTENSION"]}/vss0.dylib`,
      name: `sqlite-vss-${VERSION}-deno-darwin-aarch64.vss0.dylib`,
    },
    {
      path: `${process.env["ARTIFACT-LINUX-X86_64-EXTENSION"]}/vector0.so`,
      name: `sqlite-vss-${VERSION}-deno-linux-x86_64.vector0.so`,
    },
    {
      path: `${process.env["ARTIFACT-LINUX-X86_64-EXTENSION"]}/vss0.so`,
      name: `sqlite-vss-${VERSION}-deno-linux-x86_64.vss0.so`,
    },
  ];

  const release = await github.rest.repos.getReleaseByTag({
    owner,
    repo,
    tag: VERSION,
  });
  const release_id = release.data.id;
  const outputAssetChecksums = [];

  await Promise.all(
    compiled_extensions.map(async ({ name, path }) => {
      const data = await fs.readFile(path);
      const checksum = createHash("sha256").update(data).digest("hex");
      outputAssetChecksums.push({ name, checksum });
      return github.rest.repos.uploadReleaseAsset({
        owner,
        repo,
        release_id,
        name,
        data,
      });
    })
  );
  return outputAssetChecksums.map((d) => `${d.checksum} ${d.name}`).join("\n");
};