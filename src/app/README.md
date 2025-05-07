# Presentation and Server Layer
This folder contains the source code for the presentation and server layer:
1) Uploader Server: The server hosts the files for the presentation layer and exposes a POST endpoint that allows a user to submit a file through the UI
2) Receiver Server: The server hosts the files for the presentation layer and exposes a GET endpoint for the UI to call. The GET call will open a stream that will await the contents of the file received by the VLC receiver
3) Experiments: The code that generates random bitstreams and send them to the emitter while keeping track of what is being received


## Structure

To find the different projects, look under the directory `apps` where all the NodeJS projects will be.

The `packages` folder holds config and utily functions used to segment the data according to what is described in the report.

## How to setup monorepo
The project uses turbo repo to maintain clean and maintainable code for each of the projects. Make sure to run the following command in the root of the project:
```bash
// Assuming you have NodeJS v22.15.0 installed
$ npm run install
```

## How to run Use Case


## How to run Experiments