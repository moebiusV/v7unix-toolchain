{ lib, stdenv, fetchFromGitHub, bison, python3 }:

stdenv.mkDerivation rec {
  pname = "v7unix-toolchain";
  version = "0.1.0";

  src = fetchFromGitHub {
    owner = "moebiusV";
    repo = "v7unix-toolchain";
    rev = "v${version}";
    hash = "";  # leave empty; nix reports the correct hash on first build
  };

  nativeBuildInputs = [ bison python3 ];
  # recommended (optional): filsys, prebsd, simh

  meta = with lib; {
    description = "V7 Unix PDP-11 C toolchain, modernized for a modern host";
    homepage = "https://github.com/moebiusV/v7unix-toolchain";
    license = licenses.unfree;  # Caldera Ancient UNIX (permissive, not in the list)
    maintainers = [ maintainers.maintainer ];
    platforms = platforms.unix;
  };
}
