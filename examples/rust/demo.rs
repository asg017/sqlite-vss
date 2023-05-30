use rusqlite::{ffi::sqlite3_auto_extension, Connection, Result};
use sqlite_vss;

fn main() -> Result<()> {
    unsafe {
        sqlite3_auto_extension(Some(sqlite_vss::sqlite3_vector_init));
        sqlite3_auto_extension(Some(sqlite_vss::sqlite3_vss_init));
    }

    let conn = Connection::open_in_memory()?;
    let mut stmt = conn.prepare("SELECT 1 + 1, vss_version(), vector_to_json(X'00002842')")?;
    let mut rows = stmt.query([]).unwrap();
    let row = rows.next().unwrap().unwrap();

    let value: i32 = row.get(0).unwrap();
    println!("{:?}", value);

    let value: String = row.get(1).unwrap();
    println!("{:?}", value);

    let value: String = row.get(2).unwrap();
    println!("{:?}", value);

    Ok(())
}
