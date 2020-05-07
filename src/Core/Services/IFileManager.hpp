#ifndef CORE_SERVICE_IFILEMANAGER_HPP
#define CORE_SERVICE_IFILEMANAGER_HPP

#include <string>
#include <list>

/**
 * IFileManager is the interface which any class intending to do FileIO in
 * etterna must implement. IFileManager assumes the use of a virtual file
 * system (VFS) to abstract away from the specific filesystem/operating system.
 */
class IFileManager {
public:
    // Folder/File Mounting
    /**
     * Mounts the input file to be accessible at the given mount point.
     * @param file File, Folder, or zip file to be mounted.
     * @param mountPoint VFS mountpoint location.
     * @return true if successful, false if failed.
     */
    virtual bool mount(const std::string file, const std::string mountPoint) = 0;

    /**
     * Remove a directory/mountpoint from VFS.
     * @param location mountpoint to remove.
     * @return true if successful, false if failed.
     */
    virtual bool unmount(const std::string location) = 0;

    // File/Folder Information
    /**
     * Get a list of strings, which each string being a folder/file in the given path.
     * @param path VFS path to get directory listing.
     * @return a list of strings.
     */
    virtual std::list<std::string> getDirectoryListing(const std::string path) = 0;

    /**
     * Determine if a path is a file.
     * @param file file path to check.
     * @return true if file, false if not file.
     */
    virtual bool isFile(const std::string file) = 0;

    /**
     * Determine if a path is a folder.
     * @param file folder path to check.
     * @return true if folder, false if not folder.
     */
    virtual bool isDirectory(const std::string file) = 0;

    /**
     * Determine if a path exists. Does not ensure given path is specifically a file.
     * @param file path to check.
     * @return true if exists, false if does not exist.
     */
    virtual bool doesFileExists(const std::string file) = 0;

    // File/Filder Modification
    virtual bool folderCreate(const std::string folder) = 0;    /** @brief Create folder at given path */
    virtual bool fileCreate(const std::string file) = 0;        /** @brief Create file at given path */
    virtual bool fileDelete(const std::string file) = 0;        /** @brief Delete file at given path */
    virtual bool fileMove(const std::string oldPath, const std::string newPath) = 0;
};

#endif //CORE_SERVICE_IFILEMANAGER_HPP
