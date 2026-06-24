#include <mach-o/dyld.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv) {
    char executable_path[PATH_MAX];
    uint32_t path_size = sizeof(executable_path);
    char *macos_directory;
    char game_path[PATH_MAX];
    char resources_path[PATH_MAX];

    if (_NSGetExecutablePath(executable_path, &path_size) != 0) {
        fputs("Could not locate the application bundle.\n", stderr);
        return EXIT_FAILURE;
    }

    macos_directory = strrchr(executable_path, '/');
    if (macos_directory == NULL) {
        fputs("Invalid application executable path.\n", stderr);
        return EXIT_FAILURE;
    }
    *macos_directory = '\0';

    if (snprintf(game_path, sizeof(game_path), "%s/pilgrim-bin", executable_path) >=
            (int)sizeof(game_path) ||
        snprintf(resources_path, sizeof(resources_path), "%s/../Resources", executable_path) >=
            (int)sizeof(resources_path)) {
        fputs("Application bundle path is too long.\n", stderr);
        return EXIT_FAILURE;
    }

    if (chdir(resources_path) != 0) {
        perror("Could not open application resources");
        return EXIT_FAILURE;
    }

    if (argc > 0) {
        argv[0] = game_path;
    }
    execv(game_path, argv);
    perror("Could not launch Pilgrim of the Thorn");
    return EXIT_FAILURE;
}
