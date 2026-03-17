{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = with pkgs; [
    gcc
    curl
    pkg-config
  ];

  shellHook = ''
    export LD_LIBRARY_PATH=${pkgs.curl.out}/lib:$LD_LIBRARY_PATH
  '';
}
