{nixpkgs ? import <nixpkgs> {}}:

with nixpkgs;
with pkgs;

let
  keyfile2json = stdenv.mkDerivation {
    name = "keyfile2json";
    src = ./.;
    buildInputs = [ glib json_glib glib pkgconfig ];
    configurePhase = ''
      export FLAGS="`pkg-config --libs --cflags glib-2.0` `pkg-config --libs --cflags json-glib-1.0`"
    '';
    buildPhase = ''
     cc main.c $FLAGS
    '';
    installPhase = ''
      mkdir -p $out/bin
      cp a.out $out/bin/keyfile2json
    '';
  };
in {
  inherit keyfile2json;
}
