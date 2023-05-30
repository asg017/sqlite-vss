fn main() {
    println!("cargo:rustc-link-arg=-Wl,-undefined,dynamic_lookup");
}
