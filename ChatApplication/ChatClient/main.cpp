/** Main driver to run the client with Qt interface
 */

#include "ChatAppClient.hpp"
#include <QApplication>

/** Main driver to run the client with Qt interface
 * @param int argc: Number of command line arguments
 * @param char *argv[]: Command line arguments
 */
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Connect client to server
    ChatServerClient chatter(grpc::CreateChannel
                            ("localhost:50051"
                           , grpc::InsecureChannelCredentials()));

    // Start the client by showing the log in window
    chatter.start();
    return a.exec();
}
