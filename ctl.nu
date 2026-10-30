#!/usr/bin/env nu

# # Framework command-line tool
#
# Manage module scaffolding and Git submodules from the project root.
# Running without a subcommand prints the tool's name.
#
# ## Usage
#
# ```nu
# nu ctl.nu
# nu ctl.nu --help
# nu ctl.nu addModule --help
# ```
#
# ## Commands
#
# | Command | Description |
# | --- | --- |
# | `addModule` | Create source/header files and register a module with CMake. |
# | `linkModule` | Placeholder; does not currently link modules. |
# | `add gitSubmodule` | Add a Git submodule under `external`. |
# | `remove gitSubmodule` | Deinitialize and remove an external Git submodule. |
#
# ## Requirements
#
# Nushell is required. Git submodule commands also require `git` on `PATH`.
# Paths are relative to the current working directory, so run from the project
# root containing `src`, `includes`, and the Git repository.
def main [] {
    print "Framework's Command Line Tool";
}

# # Add a module
#
# Create module files and append their CMake declarations.
#
# ## Usage
#
# ```nu
# nu ctl.nu addModule navigation
# nu ctl.nu addModule navigation true false
# ```
#
# ## Parameters
#
# All arguments are positional; supply boolean values as `true` or `false`.
#
# | Parameter | Default | Description |
# | --- | --- | --- |
# | `module` | Required | Module name used for folders, files, and the CMake target. |
# | `shared` | `false` | Use a `SHARED` library instead of `STATIC`. |
# | `private` | `false` | Use `PRIVATE` include directories instead of `PUBLIC`. |
#
# ## Effects
#
# - Create `src/<module>/<module>.cpp`, `includes/<module>/<module>.hpp`, and a
#   `CMakeLists.txt` in each module directory.
# - Append `add_library` and `target_include_directories` declarations to the
#   module's CMake files.
# - Append `add_subdirectory` to the existing parent CMake files in `src` and
#   `includes`, then print the created and modified file paths.
#
# Use a new module name: repeated calls append duplicate CMake declarations.
def "main addModule" [module: string shared: bool = false private: bool = false] {
    let src_dir: string = (['src' $module] | path join);
    let includes_dir: string = (['includes' $module] | path join);
    let parent_src_CMakeLists: string = (['src' 'CMakeLists.txt'] | path join);
    let parent_includes_CMakeLists: string = (['includes' 'CMakeLists.txt'] | path join);
    let src_CMakeList: string = ([$src_dir 'CMakeLists.txt'] | path join);
    let includes_CMakeLists: string = ([$includes_dir 'CMakeLists.txt'] | path join);
    let library_type: string = if $shared { 'SHARED' } else { 'STATIC' };
    let library_link_type: string = if $private { 'PRIVATE' } else { 'PUBLIC' };
    let files: list<string> = [
        ([$src_dir $"($module).cpp"] | path join)
        $src_CMakeList
        ([$includes_dir $"($module).hpp"] | path join)
        $includes_CMakeLists
    ];
    let modified_files: list<string> = [
        $parent_src_CMakeLists
        $parent_includes_CMakeLists
    ];
    let add_library_statement = $"add_library\(($module) ($library_type) ($module).cpp)";
    let target_include_directories_statement = $"target_include_directories\(($module) ($library_link_type) ${CMAKE_CURRENT_SOURCE_DIR})";

    mkdir $src_dir $includes_dir;
    touch ...$files;

    $add_library_statement | save --append $src_CMakeList;
    $target_include_directories_statement | save --append $includes_CMakeLists;

    open $parent_src_CMakeLists
        | lines --skip-empty
        | str trim
        | append $"add_subdirectory\(($module))"
        | str join "\n"
        | $"($in)\n"
        | save --force $parent_src_CMakeLists
        ;
    open $parent_includes_CMakeLists
        | lines --skip-empty
        | str trim
        | append $"add_subdirectory\(($module))"
        | str join "\n"
        | $"($in)\n"
        | save --force $parent_includes_CMakeLists
        ;

    print $"(ansi green)Added module: (ansi green_bold)($module)(ansi reset)";
    print 'Created files:';
    print (
        $files | each { |file|
            {
                files: ($file | path basename)
                path: ($file | path dirname)
            }
        } | table;
    );
    print 'Modified files:';
    print (
        $modified_files | each { |file|
            {
                files: ($file | path basename)
                path: ($file | path dirname)
            }
        } | table;
    );
}

# # Link modules (placeholder)
#
# This command currently prints `uwu` and does not modify any files or targets.
#
# ## Usage
#
# ```nu
# nu ctl.nu linkModule navigation application false
# ```
#
# ## Parameters
#
# All arguments are positional and currently unused by the implementation.
#
# | Parameter | Default | Intended role |
# | --- | --- | --- |
# | `linkModule` | Required | Module to link into the target. |
# | `targetModule` | Required | Target module that would receive the dependency. |
# | `private` | `false` | Whether the dependency would be private. |
def "main linkModule" [linkModule: string targetModule: string private: bool = false] {
    print uwu;
}

# # Add a Git submodule
#
# Register and clone a repository under `external/<name>` using
# `git submodule add --force --name`.
#
# ## Usage
#
# ```nu
# nu ctl.nu add gitSubmodule <name> <gitURL>
# ```
#
# Replace the placeholders with the submodule name and repository URL or path.
# Both arguments are required and positional. Git manages the corresponding
# `.gitmodules` entry and index changes; this command does not create a commit.
def "main add gitSubmodule" [name: string gitURL: string] {
    git submodule add --force --name $name $gitURL external/($name);
}

# # Remove a Git submodule
#
# Deinitialize and remove a repository registered under `external/<name>`.
#
# ## Usage
#
# ```nu
# nu ctl.nu remove gitSubmodule <name>
# ```
#
# Replace `<name>` with the required positional submodule name.
#
# ## Effects
#
# Run `git submodule deinit --force`, then `git rm --force` for the submodule's
# path, and recursively delete `.git/modules/<name>`. These forced operations
# can discard local submodule changes and remove its local Git history.
# Git manages the index and `.gitmodules` changes; no commit is created.
def "main remove gitSubmodule" [name: string] {
    git submodule deinit --force external/($name);
    git rm --force external/($name);
    rm --recursive --force .git/modules/($name);
}
