use flate2::read::GzDecoder;
use std::{io::BufReader, path::PathBuf};
use tar::Archive;
use ureq::get;
use zip::read::ZipArchive;

enum Platform {
    MacosX86_64,
    MacosAarch64,
    LinuxX86_64,
}

fn download_static_for_platform(version: String, output_directory: &PathBuf) {
    let os = std::env::var("CARGO_CFG_TARGET_OS").expect("CARGO_CFG_TARGET_OS not found");
    let arch = std::env::var("CARGO_CFG_TARGET_ARCH").unwrap();
    let platform = match (os.as_str(), arch.as_str()) {
        ("linux", "x86_64") => Platform::LinuxX86_64,
        ("macos", "x86_64") => Platform::MacosX86_64,
        ("macos", "aarch64") => Platform::MacosAarch64,
        _ => todo!("TODO"),
    };

    let base = "https://github.com/asg017/sqlite-vss/releases/download";
    let url = match platform {
        Platform::MacosX86_64 => {
            format!("{base}/{version}/sqlite-vss-{version}-static-macos-x86_64.tar.gz")
        }

        Platform::MacosAarch64 => {
            format!("{base}/{version}/sqlite-vss-{version}-static-macos-aarch64.tar.gz")
        }
        Platform::LinuxX86_64 => {
            format!("{base}/{version}/sqlite-vss-{version}-static-linux-x86_64.tar.gz")
        }
    };

    println!("{url}");
    let response = get(url.as_str()).call().expect("Failed to download file");
    println!("{}", response.get_url());
    let mut reader = response.into_reader();

    if url.ends_with(".zip") {
        let mut buf = Vec::new();
        reader.read_to_end(&mut buf).unwrap();
        let mut archive =
            ZipArchive::new(std::io::Cursor::new(buf)).expect("Failed to open zip archive");
        archive
            .extract(output_directory)
            .expect("failed to extract .zip file");
    } else {
        let buf_reader = BufReader::new(reader);
        let decoder = GzDecoder::new(buf_reader);
        let mut archive = Archive::new(decoder);
        archive
            .unpack(output_directory)
            .expect("Failed to extract tar.gz file");
    }
}
fn main() {
    // check if the env var LIB_SQLITE_VSS exists
    let static_path = if let Ok(v) = std::env::var("LIB_SQLITE_VSS") {
        v
    } else {
        let version = format!("v{}", env!("CARGO_PKG_VERSION"));
        let output_directory = std::path::Path::new(std::env::var("OUT_DIR").unwrap().as_str())
            .join(format!("sqlite-vss-v{version}"));
        eprintln!("{}", output_directory.to_string_lossy());
        if !output_directory.exists() {
            download_static_for_platform(version, &output_directory);
        } else {
            println!("{} already exists", output_directory.to_string_lossy())
        }
        output_directory.to_string_lossy().to_string()
    };

    println!("cargo:rustc-link-search=native={}", static_path);
    println!("cargo:rustc-link-lib=static=faiss_avx2");
    println!("cargo:rustc-link-lib=static=vector0");
    println!("cargo:rustc-link-lib=static=vss0");
    println!("cargo:rustc-link-arg=-Wl,-undefined,dynamic_lookup");
}
