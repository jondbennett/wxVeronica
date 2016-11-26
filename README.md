# Veronica
A wxWidgets program to monitor and configure the Gary Cooper chicken coop controller.

The description says it all. This program monitors the telemetry data sent, via radio datalink from the Gary Cooper Arduino chicken coop controller, and provides voice updates whenever the coop door automatically opens or closes. It also provides the opportunity to change the settings as the current Gary Cooper implementation has no physical user interface.

NOTE: This program will not build on it's own. It is completely dependent on the GaryCooper project. In order to build this program you will need to install and configure wxWidgets, and Code::Blocks for the IDE. A proper directory configuration will have a base directory with the GaryCooper and Veronica directories below it. The Veronica code (this project) uses relative paths to include important shared files from the GaryCooper chicken coop controller project.
