/*
 *	Copyright (c) 2021–2025, Signaloid.
 *
 *	Permission is hereby granted, free of charge, to any person obtaining a copy
 *	of this software and associated documentation files (the "Software"), to deal
 *	in the Software without restriction, including without limitation the rights
 *	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *	copies of the Software, and to permit persons to whom the Software is
 *	furnished to do so, subject to the following conditions:
 *
 *	The above copyright notice and this permission notice shall be included in all
 *	copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *	SOFTWARE.
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
	kUtilityConstantsMaxCharsPerFilepath		= 1024,
	kUtilityConstantsError				= 1,
	kUtilityConstantsSuccess			= 0
} UtilityConstants;

/*
 *	outputPipelineMode is set to 0 if the '-o' option is provided as a command line argument, else it is set to 1.
 */
extern int	outputPipelineMode;

typedef struct CommandLineArguments
{
	char		inputFilePath[kUtilityConstantsMaxCharsPerFilepath];
	char		outputFilePath[kUtilityConstantsMaxCharsPerFilepath];
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
 *	@param	outputFilePath			: path prefix for the output CSV file to write to
 *	@param	outputVariables			: array of output distributions whose Ux representations we will write to the output CSV
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
 *	@param argc		: Number of arguments
 *	@param argv		: Array of arguments
 *	@param sampleCount	: Number of samples for the distribution
 *	@param out		: Pointer to the output distribution
 *	@return			: int 0 if successful, else 1
 */
int
processSampleList(
	int		argc,
	char *		argv[],
	int		sampleCount,
	double *	out);
