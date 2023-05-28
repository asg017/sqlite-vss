declare module 'sqlite-vss' {
  export interface Database {
    // after https://deno.land/x/sqlite3@0.8.0/mod.ts?s=Database#method_loadExtension_0
    loadExtension(file: string, entrypoint?: string | undefined): void;
  }

  /**
   * Loads the sqlite-vss extension on the given sqlite3 database.
   */
  export function loadVector(database: Database): void;
  /**
   * Loads the sqlite-vss extension on the given sqlite3 database.
   */
  export function loadVss(database: Database): void;

  /**
   * Loads the sqlite-vss extension on the given sqlite3 database.
   */
  export function load(database: Database): void;
}
