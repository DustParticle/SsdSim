var fs = require('fs');
var request = require('request');

function updateStatus(data, status, callback) {
    var url = process.env.WATERFALLSENDPOINT + '/api/dashboard/UpdateProcedureResult/status/' + status;
    request({
        url: url,
        method: 'POST',
        json: true,
        body: data,
        headers: {
            'x-api-key': process.env.MAPPED_API_KEY
        }
    }, callback);
}

var lastCommitPayload = {
    author: process.env.BUILD_SOURCEVERSIONAUTHOR,
    repositoryName: process.env.BUILD_REPOSITORY_NAME,
    branch: process.env.BUILD_SOURCEBRANCHNAME,
    commit: process.env.BUILD_SOURCEVERSION,
    message: process.env.BUILD_SOURCEVERSIONMESSAGE,
    date: "Date Under Construction",
};

// Hardcode the configuration is always "Default"
var configuration = {
    Configuration: "Default",
};

var jobParams = {
    jobId: process.env.AGENT_JOBNAME + '|' + process.env.BUILD_BUILDNUMBER + '|' + configuration.Configuration,
    jobName: process.env.AGENT_JOBNAME,
    projectName: "SsdSim", // Temperary hardcode project name
    commitSet: "", // TODO: Need to re-define commit set
    lastCommitPayload: lastCommitPayload,
    ciConfiguration: configuration,
    configName: configuration.Configuration,
    additionalConfiguration: '',
    revisionNumber: process.env.BUILD_BUILDID,
};

// TODO: We have to convert AGENT_JOBSTATUS from Azure Pipeline style to "out style"
// Should we change to be same to Azure ??????
var status = process.argv[2];
if (status === "Succeeded" || status === "SucceededWithIssues")
{
    status = "Success";
}

updateStatus(jobParams, status, function (error, response) {
    if (!error && response.statusCode == 200) {
        // Do nothing here
    } else {
        console.log(error);
    }
});