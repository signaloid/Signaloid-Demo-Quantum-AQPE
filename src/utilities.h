/*
 *	Authored 2021, Chatura Samarakoon.
 *	Authored 2023, Bilgesu Bilgin, Stelios Tsagkarakis.
 *
 *	Copyright (c) 2021--2023, Signaloid.
 *
 *	All rights reserved.
 *
 *	Redistribution and use in source and binary forms, with or without
 *	modification, are permitted provided that the following conditions
 *	are met:
 *	*	Redistributions of source code must retain the above
 *		copyright notice, this list of conditions and the following
 *		disclaimer.
 *	*	Redistributions in binary form must reproduce the above
 *		copyright notice, this list of conditions and the following
 *		disclaimer in the documentation and/or other materials
 *		provided with the distribution.
 *	*	Neither the name of the author nor the names of its
 *		contributors may be used to endorse or promote products
 *		derived from this software without specific prior written
 *		permission.
 *
 *	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *	FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *	COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *	BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *	CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *	LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *	ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *	POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

typedef enum
{
	kUtilityConstantsMaxNumberOfInputSamples	= 100000,
	kUtilityConstantsMinNumberOfInputSamples	= 1,
	kUtilityConstantsMaxCharsPerLine		= 1024 * 1024,
	kUtilityConstantsError				= 1,
	kUtilityConstantsSuccess			= 0
} UtilityConstants;

/*
 *	outputPipelineMode is set to 0 if the '-o' option is provided as a command line argument, else it is set to 1.
 */
extern int	outputPipelineMode;

typedef struct CommandLineArguments
{
	char *		inputFilePath;
	char *		outputFilePath;
	bool		outputPipelineMode;
	double		targetPhi;
	double		precision;
	double		alpha;
	uint64_t	numberOfEvidenceSamplesPerIteration;
	size_t		numberOfRepetitions;
	bool		verbose;
	bool		priorSet;
	bool		fileSet;
	double		priorInformation;
} CommandLineArguments;


/**
 *	@brief	Print out command line usage.
 */
void	printUsage(void);

/**
 *	@brief	Get command line arguments.
 *
 *	@param	argc		: argument count from main()
 *	@param	argv		: argument vector from main()
 *	@param	arguments	: Pointer to struct to store arguments
 *	@return	int		: 0 if successful, else 1
 */
int
getCommandLineArguments(
	int			argc,
	char *			argv[],
	CommandLineArguments *	arguments);

/**
 *	@brief	Read data from a CSV file. Data entries are wither numbers or Ux-values.
 *
 *	@param	inputFilePath			: path to CSV file to read from
 *	@param	inputDistributions		: array of input distributions to be obtained from the read CSV data
 *	@param	numberOfInputDistributions	: number of input distributions to be obtained from the read CSV data
 *	@return	int				: 0 if successful, else 1
 */
int
readInputDistributionsFromCSV(
	char *		inputFilePath,
	double *	inputDistributions,
	size_t		numberOfInputDistributions);

/**
 *	@brief	Write Ux-valued data to a CSV file.
 *
 *	@param	outputFilePath		: path prefix for the output CSV file to write to
 *	@param	outputVariables		: array of output distributions whose Ux representations we will write to the output CSV
 *	@param	numberOfOutputDistributions	: number of output distributions whose Ux representations we will write to the output CSV
 *	@param	outputVariableNames		: names of output distributions whose Ux representations we will write to the output CSV
 *	@return	int				: 0 if successful, else 1
 */
int
writeOutputDistributionsToCSV(
	char *		outputFilePath,
	double *	outputVariables,
	char *		outputVariableNames[],
	size_t		numberOfOutputDistributions);

/**
 *	Process a list of weighted samples and store the output distribution in the out array.
 *	@param argc Number of arguments
 *	@param argv Array of arguments
 *	@param sampleCount Number of samples for the distribution
 *	@param out Pointer to the output distribution
 *	@return int 0 if successful, else 1
 */
int
processSampleList(
	int		argc,
	char *		argv[],
	int		sampleCount,
	double *	out);
