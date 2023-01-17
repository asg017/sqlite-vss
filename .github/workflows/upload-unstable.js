const fs = require("fs").promises;

module.exports = async ({ github, context }) => {
  const {
    repo: { owner, repo },
    sha,
  } = context;
  console.log(process.env.GITHUB_REF);
  const release = await github.rest.repos.getReleaseByTag({
    owner,
    repo,
    tag: "unstable",
  });

  const release_id = release.data.id;
  async function uploadReleaseAsset(path, name) {
    console.log("Uploading", name, "at", path);

    return github.rest.repos.uploadReleaseAsset({
      owner,
      repo,
      release_id,
      name,
      data: await fs.readFile(path),
    });
  }
  await Promise.all([
    uploadReleaseAsset("sqlite-vss-ubuntu/vss0.so", "vss0.so"),
    //uploadReleaseAsset("sqlite-vss/sqlite-vss-macos/vss0.dylib", "vss0.dylib"),
    //uploadReleaseAsset("sqlite-xsv-windows/xsv0.dll", "xsv0.dll"),
  ]);

  return;
};
