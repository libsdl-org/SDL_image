const octokit = require("@octokit/rest");
const core = require("@actions/core");
const fs = require("fs");
const os = require("os");
const path = require("path");
const process = require("process");
const shockpkg = require("@shockpkg/archive-files");

const GITHUB = new octokit.Octokit({auth: process.env.GITHUB_TOKEN});

async function log_rate_limit() {
  response = await GITHUB.rateLimit.get();

  console.log(`Rate limit: ${response.data.resources.core.remaining}/${response.data.resources.core.limit} remaining`);
}

log_rate_limit();


const WORKDIR = fs.mkdtempSync(path.join(os.tmpdir(), "sdl-"));
process.chdir(WORKDIR);
console.log(`Work directory: ${WORKDIR}`);

// Don't look further then 120 entries in the past
const MAX_PAGINATION = 120;

async function find_artifact_id(owner, repo, name) {
  var artifacts_seen = 0;
  console.log({owner: owner, repo: repo, name: name});
  for await (const artifactsPage of GITHUB.paginate.iterator(GITHUB.rest.actions.listArtifactsForRepo, {owner: owner, repo: repo, name: name})) {
    for (artifact of artifactsPage.data) {
      if (!artifact.expired) {
        console.log(`Found artifact id ${artifact.id}`);
        return artifact.id;
      }
    }
    artifacts_seen += artifactsPage.data.length;
    if (artifacts_seen >= MAX_PAGINATION) {
      core.error(`Could not find artifact after ${ artifacts_seen } artifacts. (max=${MAX_PATINATION})`);
      return;
    }
  }
  core.error(`Could not find artifact.`);
  return;
}

async function download_artifact(owner, repo, name) {
  artifact_id = await find_artifact_id(owner, repo, name);
  if (artifact_id == undefined) { return; }
  artifact = await GITHUB.rest.actions.downloadArtifact({owner: owner, repo: repo, artifact_id: artifact_id, archive_format: "zip"});
  const zipFilePath = path.join(WORKDIR, `${name}.zip`);
  fs.writeFileSync(zipFilePath, Buffer.from(artifact.data));
  console.log(`Downloaded artifact to ${zipFilePath} (${artifact.data.byteLength} bytes)`)
  return zipFilePath;
}

async function extract_archive_to(archive_path, dest_path) {
  console.log(`Extracting ${archive_path} to ${dest_path}.`);
  const archiver = new shockpkg.createArchiveByFileExtension(archive_path);
  await archiver.read(async entry => {
      const extractPath = path.join(dest_path, entry.path);
      console.log(`... ${extractPath}`);
      await entry.extract(extractPath);
    }
  );
}

async function configure_artifact(owner, repo, name) {
  const zipPath = await download_artifact(owner, repo, name);
  if (zipPath == undefined) { return; }

  // Extract downloaded .zip artifact to `extracted-zip`
  const extractedZipPath = path.join(WORKDIR, "extracted-zip");
  await extract_archive_to(zipPath, extractedZipPath);

  // Extract every archive inside .zip artifact to `extracted`
  const zipContents = fs.readdirSync(extractedZipPath).filter(s => s.startsWith("SDL"));
  const extractPath = path.join(WORKDIR, "extracted");
  fs.mkdirSync(extractPath);
  for (extractedFileFromZip of zipContents) {
    await extract_archive_to(path.join(extractedZipPath, extractedFileFromZip), extractPath);
  }

  const sdl_root = path.join(extractPath, fs.readdirSync(extractPath)[0]).replaceAll("\\", "/");
  console.log(`sdl_root = ${sdl_root}.`);
  core.setOutput("sdl_root", sdl_root);
}

const owner = core.getInput("owner", { required: true });
const repo = core.getInput("repo", { required: true });
const artifact_name = core.getInput("artifact_name", { required: true });

configure_artifact(owner, repo, artifact_name);
