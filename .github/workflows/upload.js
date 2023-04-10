const fs = require("fs").promises;
const crypto = require("crypto");
const zlib = require("zlib");
const tar = require("tar-fs");
const { basename } = require("path");

const PROJECT = "sqlite-vss";
const extension = {
  name: "vss0",
  description: "",
  platforms: [
    {
      paths: [
        "sqlite-vss-linux_x86/vss0.so",
        "sqlite-vss-linux_x86/vector0.so",
      ],
      os: "linux",
      cpu: "x86_64",
    },
    {
      paths: ["sqlite-vss-macos/vector0.dylib", "sqlite-vss-macos/vss0.dylib"],
      os: "darwin",
      cpu: "x86_64",
    },
  ],
};

async function targz(files) {
  console.log("targz files: ", files[0].name, files[0]);

  const tarStream = tar.pack();

  for (const file of files) {
    await new Promise((resolve, reject) => {
      tarStream.entry({ name: file.name }, file.data, (err) => {
        if (err) reject(err);
        else resolve();
      });
    });
  }

  tarStream.finalize();

  const gzip = zlib.createGzip();

  const chunks = [];
  return await new Promise((resolve, reject) => {
    tarStream
      .pipe(gzip)
      .on("data", (chunk) => {
        chunks.push(chunk);
      })
      .on("end", () => {
        const buffer = Buffer.concat(chunks);
        resolve(buffer);
      })
      .on("error", reject);
  });
}

module.exports = async ({ github, context }) => {
  const {
    repo: { owner, repo },
    sha,
  } = context;
  const VERSION = process.env.GITHUB_REF_NAME;

  const release = await github.rest.repos.getReleaseByTag({
    owner,
    repo,
    tag: process.env.GITHUB_REF.replace("refs/tags/", ""),
  });
  const release_id = release.data.id;

  async function uploadPlatform(platform) {
    const { paths, os, cpu } = platform;

    const tar = await targz(
      await Promise.all(
        paths.map(async (path) => ({
          name: basename(path),
          contents: await fs.readFile(path),
        }))
      )
    );

    const asset_name = `${PROJECT}-${VERSION}-${os}-${cpu}.tar.gz`;
    const asset_md5 = crypto.createHash("md5").update(tar).digest("base64");
    const asset_sha256 = crypto.createHash("sha256").update(tar).digest("hex");

    await github.rest.repos.uploadReleaseAsset({
      owner,
      repo,
      release_id,
      name: asset_name,
      data: tar,
    });

    return {
      os,
      cpu,
      asset_name,
      asset_sha256,
      asset_md5,
    };
  }

  const spm_json = {
    version: 0,
    extensions: {
      [extension.name]: {
        description: extension.description,
        platforms: await Promise.all(
          extension.platforms.map((platform) => uploadPlatform(platform))
        ),
      },
    },
  };

  await github.rest.repos.uploadReleaseAsset({
    owner,
    repo,
    release_id,
    name: "spm.json",
    data: JSON.stringify(spm_json),
  });
};
