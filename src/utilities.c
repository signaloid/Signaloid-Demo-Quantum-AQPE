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

#include <math.h>
#include <ctype.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <uxhw.h>
#include "utilities.h"

const char *		kDefaultInputFilePath = "input.csv";
const double		kMinimumAlpha = 0.0;
const double		kMaximumAlpha = 1.0;
const double		kMinimumPhi = -M_PI;
const double		kMaximumPhi = M_PI;
const double		kMinimumPrecision = 1e-10;
static const char * 	optArgsString = ":s:i:o:t:p:a:n:r:vh";
const double		kMaximumPrecision = 1.0;
const uint64_t		kMaximumNumberOfEvidenceSamples = kUtilityConstantsMaxNumberOfInputSamples;

void
printUsage(void)
{
	fprintf(stdout, "Example: Accelerated Quantum Phase Estimation (AQPE)\n");
	fprintf(stdout, "\n");
	fprintf(stdout, "Command line arguments:\n");
	fprintf(stdout,
		"[-s <K> <sample1> <sample2> ... <sampleK> <sampleWeight1> <sampleWeight2> ... <sampleWeightK>] (K in [%d, %d])\n"
		"[-i [path_to_input_csv_file : str] (Default: '%s')]\n"
		"[-o <path_to_output_csv_file : str>]\n"
		"[-t <target_phase : double in [-pi, pi]>] (Default: pi / 2)\n"
		"[-p <precision_in_phase_estimation : double in [%"SignaloidParticleModifier"le, %"SignaloidParticleModifier"le]>] (Default: 1e-4)\n"
		"[-a <alpha : double in [0,1]>] (Default: 0.5)\n"
		"[-n <number_of_evidence_samples_per_iteration : uint64_t in [0, inf)>] (Default: see README.md)\n"
		"[-r <number_of_repetitions : size_t in (0, inf)>] (Default: 1)\n"
		"[-v] (Verbose mode: Prints details of each repeated AQPE experiment to stdout.)\n"
		"[-h] (Display this help message.)\n",
		kUtilityConstantsMinNumberOfInputSamples,
		kUtilityConstantsMaxNumberOfInputSamples,
		kDefaultInputFilePath,
		kMinimumPrecision,
		kMaximumPrecision);
	fprintf(stdout, "\n");
}


int
getCommandLineArguments(
	int			argc,
	char *			argv[],
	CommandLineArguments *	arguments)
{
	int	opt;
	bool	userSpecifiedEvidenceNumber = false;
	double	sampleCount;
	char *	end;

	opterr = 0;

	while ((opt = getopt(
			argc,
			argv,
			optArgsString
			)) != EOF)
	{
		/*
		 *	Workaround to enable optional option arguments.
		 */
		if (optarg != NULL)
		{
			if (!strncmp(optarg, "-", 1))
			{
				char * colon = ":";
				if (strlen(optarg) > 1)
				{
					if (!isdigit((int) (optarg[1])))
					{
						optarg = "";
						optopt = opt;
						opt = (int) (colon[0]);
						optind--;
					}
				}
				else
				{
					fprintf(stderr, "Error: Illegal argument %s for option -%c.\n", optarg, opt);

					return kUtilityConstantsError;
				}
			}
		}

		switch (opt)
		{
			case 's':
			{
				/**
				 *	Reset errno before calling strtod.
				 */
				errno = 0;
				sampleCount = strtol(optarg, &end, 10);

				if (errno != 0 || end == optarg || strcmp(end, "\0") != 0)
				{
					fprintf(stderr, "Error: The argument of option -%c (number of samples) should be a valid number.\n", opt);
					return kUtilityConstantsError;
				}

				if (sampleCount < kUtilityConstantsMinNumberOfInputSamples ||
				    sampleCount > kUtilityConstantsMaxNumberOfInputSamples)
				{
					fprintf(stderr, "Error: The argument of option -%c (number of samples) should be a non-negative integer, between %d and %d.\n",
						opt, kUtilityConstantsMinNumberOfInputSamples, kUtilityConstantsMaxNumberOfInputSamples);
					return kUtilityConstantsError;
				}
				if (optind + 2*sampleCount > argc)
				{
					fprintf(stderr, "Error: Invalid number of arguments. Expected %f samples and %f weights, got less.\n",
						sampleCount, sampleCount);
					return kUtilityConstantsError;
				}

				int outcome = processSampleList(
							argc,
							argv,
							sampleCount,
							&arguments->priorInformation);
				if (outcome != 0)
				{
					return outcome;
				}
				/**
				 *	Check if the user has already provided file as input.
				 *	If so, print an error message and return.
				 */
				if (arguments->fileSet)
				{
					fprintf(stderr, "Error: Define either samples (-s) or input file (-i), not both.\nIf you are using the web-based application, select \"Initial prior from file\" to use -i.\nThe slider widget automatically generates samples and uses the -s option.");
					return kUtilityConstantsError;
				}
				else
				{
					arguments->priorSet = true;
					sprintf(arguments->inputFilePath, "-");
				}
				break;
			}
			case 'i':
			{
				sprintf(arguments->inputFilePath, "%s", optarg);
				/**
				 *	Check if the user has already provided samples as input.
				 *	If so, print an error message and exit.
				 */
				if (arguments->priorSet)
				{
					fprintf(stderr, "Error: Define either samples (-s) or input file (-i), not both.\nIf you are using the web-based application, select \"Initial prior from file\" to use -i.\nThe slider widget automatically generates samples and uses the -s option.\n");
					return kUtilityConstantsError;
				}
				break;
			}
			case 'o':
			{
				sprintf(arguments->outputFilePath, "%s", optarg);
				arguments->writeOutputToFile = true;
				break;
			}
			case 't':
			{
				errno = 0;
				arguments->targetPhi = strtod(optarg, &end);
				if (errno != 0 || end == optarg || strcmp(end, "\0") != 0)
				{
					fprintf(stderr, "Error: The argument of option -%c (target phase) should be a valid number.\n", opt);
					return kUtilityConstantsError;
				}

				if ((arguments->targetPhi < kMinimumPhi) || (arguments->targetPhi > kMaximumPhi))
				{
					fprintf(stderr, "Warning: The argument of option -%c (precision) should be in [%le, %le]. Continuing with the default value %le.\n", opt, kMinimumPhi, kMaximumPhi, arguments->targetPhi);
				}
				break;
			}
			case 'p':
			{
				errno = 0;
				double value = strtod(optarg, &end);
				if (errno != 0 || end == optarg)
				{
					fprintf(stderr, "Error: The argument of option -%c (precision) should be a valid number.\n", opt);
					return kUtilityConstantsError;
				}

				if ((value < kMinimumPrecision) ||
				    (value > kMaximumPrecision))
				{
					fprintf(stderr, "Warning: The argument of option -%c (precision) should be in [%le, %le]. Continuing with the default value %le.\n", opt, kMinimumPrecision, kMaximumPrecision, arguments->precision);
				}
				else
				{
					arguments->precision = value;
				}
				break;
			}
			case 'a':
			{
				errno = 0;
				arguments->alpha = strtod(optarg, &end);
				if (errno != 0 || end == optarg || strcmp(end, "\0") != 0)
				{
					fprintf(stderr, "Error: The argument of option -%c (alpha) should be a valid number.\n", opt);
					return kUtilityConstantsError;
				}

				if ((arguments->alpha < kMinimumAlpha) || (arguments->alpha > kMaximumAlpha))
				{
					fprintf(stderr, "Warning: The argument of option -%c (precision) should be in [%le, %le]. Continuing with the default value %le.\n", opt, kMinimumAlpha, kMaximumAlpha, arguments->alpha);
				}
				break;
			}
			case 'n':
			{
				userSpecifiedEvidenceNumber = true;
				errno = 0;
				arguments->numberOfEvidenceSamplesPerIteration = (uint64_t) strtol(optarg, &end, 10);
				if (errno != 0 || end == optarg)
				{
					fprintf(stderr, "Error: The argument of option -%c (number of samples per Bayesian inference iteration) should be a valid number.\n", opt);
					return kUtilityConstantsError;
				}

				if (arguments->numberOfEvidenceSamplesPerIteration < 0)
				{
					fprintf(stderr, "Error: The argument of option -%c (number of samples per Bayesian inference iteration) should be a non-negative integer. Use '-%c 0' to trigger automatic selection.\n", opt, opt);

					return kUtilityConstantsError;
				}

				break;
			}
			case 'r':
			{
				errno = 0;
				arguments->numberOfRepetitions =  (size_t) strtol(optarg, &end, 10);
				if (errno != 0 || end == optarg || strcmp(end, "\0") != 0)
				{
					fprintf(stderr, "Error: The argument of option -%c (number of repetitions of the AQPE experiment) should be a valid number.\n", opt);
					return kUtilityConstantsError;
				}

				if (arguments->numberOfRepetitions <= 0)
				{
					fprintf(stderr, "Error: The argument of option -%c (number of repetitions of the AQPE experiment) should be a positive integer.\n", opt);

					return kUtilityConstantsError;
				}

				break;
			}
			case 'v':
			{
				arguments->verbose = true;
				break;
			}
			case 'h':
			{
				printUsage();
				exit(0);
			}
			case ':':
			{
				switch (optopt)
				{
					case 'i':
					{
						snprintf(arguments->inputFilePath, kUtilityConstantsMaxCharsPerFilepath, "%s", (char *) kDefaultInputFilePath);
						/**
						 *	Check if the user has already provided samples as input.
						 *	If so, print an error message and exit.
						 */
						if (arguments->priorSet)
						{
							fprintf(stderr, "Error: Define either samples (-s) or input file (-i), not both.\n");
							return kUtilityConstantsError;
						}
						break;
					}
					default:
					{
						fprintf(stderr, "Error: Option -%c is missing a required argument.\n", optopt);
						printUsage();

						return kUtilityConstantsError;
					}
				}
				break;
			}
			case '?':
			{
				fprintf(stderr, "Error: Invalid option: -%c.\n", optopt);
				printUsage();

				return kUtilityConstantsError;
			}
		}
	}

	if (arguments->numberOfEvidenceSamplesPerIteration == 0)
	{
		if (arguments->alpha == 1.0)
		{
			arguments->numberOfEvidenceSamplesPerIteration = (uint64_t) ceil(4 * log(1 / arguments->precision));
		}
		else
		{
			arguments->numberOfEvidenceSamplesPerIteration = (uint64_t) ceil((2 / (1 - arguments->alpha)) * (1 / pow(arguments->precision, 2 * (1 - arguments->alpha)) - 1));
		}

		if ((!userSpecifiedEvidenceNumber) && (arguments->numberOfEvidenceSamplesPerIteration > kMaximumNumberOfEvidenceSamples))
		{
			fprintf(stderr, "Warning: The number of samples required from the quantum circuit, N = %"PRIu64", has exceeded the allowed maximum limit of %"PRIu64" samples. Using the maximum allowed.\n", arguments->numberOfEvidenceSamplesPerIteration, kMaximumNumberOfEvidenceSamples);
			fprintf(stderr, "Note: Use '-n 0' to permit the use of high default number of samples. You can also specify custom number of samples by using the '-n' command-line argument option, e.g., '-n %"PRIu64"'.\n", 10 * kMaximumNumberOfEvidenceSamples);
			arguments->numberOfEvidenceSamplesPerIteration = kMaximumNumberOfEvidenceSamples;
		}
	}

	if (arguments->verbose)
	{
		printf("In verbose mode!\n");
	}
	
	printf("inputFilePath = %s\n", arguments->inputFilePath);

	if (arguments->writeOutputToFile)
	{
		printf("outputFilePath = %s\n", arguments->outputFilePath);
	}

	printf("targetPhi = %"SignaloidParticleModifier"lf\n", arguments->targetPhi);
	printf("alpha = %"SignaloidParticleModifier"lf\n", arguments->alpha);
	printf("precision = %"SignaloidParticleModifier"le\n", arguments->precision);
	printf("numberOfEvidenceSamplesPerIteration = %"PRIu64"\n", arguments->numberOfEvidenceSamplesPerIteration);
	printf("numberOfRepetitions = %zu\n", arguments->numberOfRepetitions);
	printf("\nRequired Quantum Circuit Depth (D) = %"PRIu64"\n", (uint64_t) ceil(1 / pow(arguments->precision, arguments->alpha)));
	printf("\nRequired Quantum Circuit Samples (N) = %"PRIu64"\n", (arguments->alpha == 1.0) ? (uint64_t) ceil(4 * log(1 / arguments->precision)) : (uint64_t) ceil((2 / (1 - arguments->alpha)) * (1 / pow(arguments->precision, 2 * (1 - arguments->alpha)) - 1)));

	return kUtilityConstantsSuccess;
}


int
readInputDistributionsFromCSV(
	char *		inputFilePath,
	double *	inputDistributions,
	size_t		numberOfInputDistributions)
{
	double		inputSampleValues[numberOfInputDistributions][kUtilityConstantsMaxNumberOfInputSamples];
	char		buffer[kUtilityConstantsMaxCharsPerLine];
	char *		token;
	double		value;
	size_t		rowCount = -1;
	size_t		columnCount;
	size_t		sampleCounts[numberOfInputDistributions];
	size_t		uxColumns[numberOfInputDistributions];
	FILE *		fp = NULL;

	for (size_t i = 0; i < numberOfInputDistributions; i++)
	{
		sampleCounts[i] = 0;
		uxColumns[i] = 0;
	}

	if (strcmp(inputFilePath, "stdin"))
	{
		fp = fopen(inputFilePath, "r");

		if (fp == NULL)
		{
			fprintf(stderr, "Error: Cannot open the file %s.\n", inputFilePath);

			return kUtilityConstantsError;
		}
	}
	else
	{
		/*
		 *	TEMPORARY: Pipeline not supported
		 */
		fprintf(stderr, "Error: Pipeline mode temporarily unavailable. Please use the '-i' command-line argument option.\n");

		return kUtilityConstantsError;

		/*
		 *	Guard against empty stdin, in which case switch to the default input file.
		 *
		if (ftell(stdin) == -1)
		{
			fprintf(stderr, "\nStandard input is empty. Switching to the defalut input file %s.\n", kDefaultInputFilePath);
			fp = fopen(kDefaultInputFilePath, "r");

			if (fp == NULL)
			{
				fprintf(stderr, "Error: Cannot open the file %s.\n", kDefaultInputFilePath);
				return kUtilityConstantsError;
			}
		}
		else
		{
			fp = stdin;
		}
		*/
	}

	/*
	 *	Read the lines into buffer.
	 */
	while (fgets(buffer, kUtilityConstantsMaxCharsPerLine, fp))
	{
		/*
		 *	Skip the row containing field/column names.
		 */
		if (rowCount == -1)
		{
			rowCount++;
			continue;
		}

		columnCount = 0;
		token = strtok(buffer, ", ");

		while (token)
		{
			if (columnCount == numberOfInputDistributions)
			{
				fprintf(stderr, "Error: The input CSV data has more than expected entries at data row %zu.\n", rowCount);

				return kUtilityConstantsError;
			}

			if (uxColumns[columnCount] == 1)
			{
				token = strtok(NULL, ", ");
				columnCount++;

				continue;
			}

			if ((rowCount == 0) && (strstr(token, "Ux") != NULL))
			{
				uxColumns[columnCount] = 1;
				sscanf(token, "%lf", &inputDistributions[columnCount]);

				continue;
			}

			value = strtod(token, NULL);

			if (!strcmp(token, "-"))
			{
			}
			else if ((strncmp(token, "0", 1)) && (value == 0))
			{
				fprintf(stderr, "Error: The input CSV data at row %zu and column %zu is not a valid number.\n", rowCount, columnCount);

				return kUtilityConstantsError;
			}
			else
			{
				inputSampleValues[columnCount][sampleCounts[columnCount]] = value;
				sampleCounts[columnCount]++;
			}

			token = strtok(NULL, ", ");
			columnCount++;
		}

		if (columnCount != numberOfInputDistributions)
		{
			fprintf(stderr, "Error: The input CSV data has less than expected entries at data row %zu.\n", rowCount);

			return kUtilityConstantsError;
		}

		rowCount++;
	}

	if (fp != stdin)
	{
		fclose(fp);
	}

	/*
	 *	Get distributions from collected sample values.
	 */
	for (size_t i = 0; i < numberOfInputDistributions; i++)
	{
		if (uxColumns[i])
		{
			continue;
		}

		inputDistributions[i] = UxHwDoubleDistFromSamples(inputSampleValues[i], sampleCounts[i]);
	}

	return kUtilityConstantsSuccess;
}


int
writeOutputDistributionsToCSV(
	char *		outputFilePath,
	double *	outputVariables,
	char *		outputVariableNames[],
	size_t		numberOfOutputDistributions)
{
	FILE *	fp = NULL;

	if (strcmp(outputFilePath, "stdout"))
	{
		fp = fopen(outputFilePath, "w");

		if (fp == NULL)
		{
			fprintf(stderr, "Error: Cannot open the file %s.\n", outputFilePath);

			return kUtilityConstantsError;
		}
	}
	else
	{
		fp = stdout;
	}

	for (size_t i = 0; i < numberOfOutputDistributions; i++)
	{
		fprintf(fp, "%s", outputVariableNames[i]);
		if (i != numberOfOutputDistributions - 1)
		{
			fprintf(fp, ", ");
		}
	}
	fprintf(fp, "\n");

	for (size_t i = 0; i < numberOfOutputDistributions; i++)
	{
		fprintf(fp, "%le", outputVariables[i]);
		if (i != numberOfOutputDistributions - 1)
		{
			fprintf(fp, ", ");
		}
	}
	fprintf(fp, "\n");

	if (fp != stdout)
	{
		fclose(fp);
	}

	return kUtilityConstantsSuccess;
}


int
processSampleList(
	int		argc,
	char *		argv[],
	int		sampleCount,
	double *	out)
{
	char *			end;
	WeightedDoubleSample	weightedSamples[sampleCount];

	for (int i = 0; i < sampleCount; i++)
	{
		errno = 0; // reset errno
		weightedSamples[i].sample = strtod(argv[optind++], &end);

		/**
		 *	If the sample is not a valid number
		 *	or we are in the last sample in the list and end variable is not '\0'
		 *	then print an error message and return.
		 */
		if (errno != 0 || strcmp(end, "\0") != 0)
		{
			fprintf(stderr, "Error: Invalid sample at position %d.\n", i+1);
			return kUtilityConstantsError;
		}
	}

	for (int i = 0; i < sampleCount; i++)
	{
		errno = 0; // reset errno
		weightedSamples[i].sampleWeight = strtod(argv[optind++], &end);
		if (errno != 0 || strcmp(end, "\0") != 0)
		{
			fprintf(stderr, "Error: Invalid weight at position %d.\n", i+1+sampleCount);
			return kUtilityConstantsError;
		}
	}

	*out = UxHwDoubleDistFromWeightedSamples(
		weightedSamples,
		sampleCount);

	return kUtilityConstantsSuccess;
}
