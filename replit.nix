{ pkgs }: {
	deps = [
   pkgs.nlohmann_json_schema_validator
   pkgs.nlohmann_json
		pkgs.clang
		pkgs.ccls
		pkgs.gdb
		pkgs.gnumake
	];
}