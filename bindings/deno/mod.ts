import { download } from "https://deno.land/x/plug@1.0.1/mod.ts";
import meta from "./deno.json" assert { type: "json" };

const BASE = `${meta.github}/releases/download/v${meta.version}`;

// Similar to https://github.com/denodrivers/sqlite3/blob/f7529897720631c2341b713f0d78d4d668593ea9/src/ffi.ts#L561
let vssPath: string;
let vectorPath: string;
try {
  const customVectorPath = Deno.env.get("DENO_SQLITE_VECTOR_PATH");
  if (customVectorPath) vectorPath = customVectorPath;
  else {
    vectorPath = await download({
      url: {
        darwin: {
          aarch64: `${BASE}/sqlite-vss-v${meta.version}-deno-darwin-aarch64.vector0.dylib`,
          x86_64: `${BASE}/sqlite-vss-v${meta.version}-deno-darwin-x86_64.vector0.dylib`,
        },
        linux: {
          x86_64: `${BASE}/sqlite-vss-v${meta.version}-deno-linux-x86_64.vector0.so`,
        },
      },
      suffixes: {
        darwin: "",
        linux: "",
        windows: "",
      },
    });
  }

  const customVssPath = Deno.env.get("DENO_SQLITE_VSS_PATH");
  if (customVssPath) vssPath = customVssPath;
  else {
    vssPath = await download({
      url: {
        darwin: {
          aarch64: `${BASE}/sqlite-vss-v${meta.version}-deno-darwin-aarch64.vss0.dylib`,
          x86_64: `${BASE}/sqlite-vss-v${meta.version}-deno-darwin-x86_64.vss0.dylib`,
        },
        linux: {
          x86_64: `${BASE}/sqlite-vss-v${meta.version}-deno-linux-x86_64.vss0.so`,
        },
      },
      suffixes: {
        darwin: "",
        linux: "",
        windows: "",
      },
    });
  }
} catch (e) {
  if (e instanceof Deno.errors.PermissionDenied) {
    throw e;
  }

  const error = new Error("Failed to load sqlite-vss extension");
  error.cause = e;

  throw error;
}

/**
 * Returns the full path to the compiled sqlite-vss extension.
 * Caution: this will not be named "vss0.dylib|so|dll", since plug will
 * replace the name with a hash.
 */
export function getVssLoadablePath(): string {
  return vssPath;
}

/**
 * Returns the full path to the compiled sqlite-vector extension.
 * Caution: this will not be named "vss0.dylib|so|dll", since plug will
 * replace the name with a hash.
 */
export function getVectorLoadablePath(): string {
  return vectorPath;
}

/**
 * Entrypoint name for the sqlite-vss extensions.
 */
export const vssEntrypoint = "sqlite3_vss_init";
export const vectorEntrypoint = "sqlite3_vector_init";

interface Db {
  // after https://deno.land/x/sqlite3@0.8.0/mod.ts?s=Database#method_loadExtension_0
  loadExtension(file: string, entrypoint?: string | undefined): void;
}

/**
 * Loads the sqlite-vss extension on the given sqlite3 database.
 */
export function loadVector(db: Db): void {
  db.loadExtension(vectorPath, vectorEntrypoint);
}
/**
 * Loads the sqlite-vss extension on the given sqlite3 database.
 */
export function loadVss(db: Db): void {
  db.loadExtension(vssPath, vssEntrypoint);
}

/**
 * Loads the sqlite-vss extension on the given sqlite3 database.
 */
export function load(db: Db): void {
  loadVector(db);
  loadVss(db);
}
