# FloCore
The execution backend for VulcanForms Inc business logic.

The core can be run locally or on a server, and provided with a dependency graph of components to exectute, it will make execution decisions based on compute resources available to it.

The general execution flow might be:
1. Discover available compute units attached by network.
2. Discover available components on each compute unit and locally.
3. Wait for client connection.
4. When client connects, send list of available components.
5. Receive dependency graph or sub-graph from client.
6. Transfer all data from URIs that are attached to the dependency graph to the appropriate compute units.
7. Start component executables (could map many components to one executable with flags).
8. Track progress and respond to any client requests about progress
9. Collect results and wait for client to request.
10. Transfer results back to client.
11. Goto 3.
