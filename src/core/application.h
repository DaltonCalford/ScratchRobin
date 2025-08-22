#ifndef SCRATCHROBIN_APPLICATION_H
#define SCRATCHROBIN_APPLICATION_H

#include <memory>
#include <string>

namespace scratchrobin {

class ConnectionManager;
class MetadataManager;

class Application {
public:
    Application(ConnectionManager* connectionManager, MetadataManager* metadataManager);
    ~Application();

    // Application lifecycle
    int run();
    void shutdown();
    bool isRunning() const;

    // Application state
    void setApplicationName(const std::string& name);
    std::string getApplicationName() const;

    void setApplicationVersion(const std::string& version);
    std::string getApplicationVersion() const;

    // Component access
    ConnectionManager* getConnectionManager() const;
    MetadataManager* getMetadataManager() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // Disable copy and assignment
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_APPLICATION_H
