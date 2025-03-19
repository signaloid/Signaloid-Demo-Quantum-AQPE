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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uxhw.h>
#include "utilities.h"


typedef enum
{
	kNumberOfOutputDistributions	= 2,
	kMaxNumberOfIterations		= 100,
	kMaxNumberOfBatchSamples	= 100000,
} Constants;

typedef struct
{
	double	currentM;
	double	currentTheta;
} EvidenceModelParameters;

double
calculateM(
	double	standardDeviation,
	double	alpha);

double
calculateTheta(
	double	meanValue,
	double	standardDeviation);

double
evidenceModel(
	void *	evidenceModelParameters,
	double	phiPriorValue);

void
runQPECircuit(
	double			targetPhi,
	double *		evidenceDistribution,
	uint64_t		numberOfEvidenceSamples,
	EvidenceModelParameters	evidenceModelParameters);

bool
runAQPEExperiment(
	double			initialPrior,
	CommandLineArguments *	arguments,
	size_t			experimentNo,
	size_t *		convergenceIterationCount,
	double *		estimatedPhi);

double
calculateM(
	double	standardDeviation,
	double	alpha)
{
	if (standardDeviation == 0.0)
	{
		return 1.0;
	}
	else
	{
		return 1 / pow(standardDeviation, alpha);
	}
}

double
calculateTheta(
	double	meanValue,
	double	standardDeviation)
{
	return meanValue - standardDeviation;
}

double
evidenceModel(
	void *	evidenceModelParameters,
	double	phiPrior)
{
	EvidenceModelParameters *	evidenceModelParametersCast;
	double				zeroProbability;
	double				mixture;

	evidenceModelParametersCast = (EvidenceModelParameters *)(evidenceModelParameters);
	zeroProbability = (1 + cos(evidenceModelParametersCast->currentM * (phiPrior - evidenceModelParametersCast->currentTheta))) / 2;
	mixture = UxHwDoubleMixture(0.0, 1.0, zeroProbability);

	return mixture;
}

void
runQPECircuit(
	double			targetPhi,
	double *		evidenceDistribution,
	uint64_t		numberOfEvidenceSamples,
	EvidenceModelParameters	evidenceModelParameters)
{
	double			probabilityEvidence0GivenPhiPrior;
	double			tempUniformDistribution;
	double			uniformSampleArray[kMaxNumberOfBatchSamples];
	double			zeroEvidenceProbability;
	uint64_t		zeroEvidenceCount = 0.0;
	uint64_t		numberOfBatchSamples;
	uint64_t		remainingNumberOfSamples = numberOfEvidenceSamples;
	WeightedDoubleSample	evidenceWeightedSamples[2];

	probabilityEvidence0GivenPhiPrior = (1 + cos(evidenceModelParameters.currentM * (targetPhi - evidenceModelParameters.currentTheta))) / 2;
	tempUniformDistribution = UxHwDoubleUniformDist(0.0, 1.0);

	while (remainingNumberOfSamples > 0)
	{
		if (remainingNumberOfSamples > kMaxNumberOfBatchSamples)
		{
			numberOfBatchSamples = kMaxNumberOfBatchSamples;
		}
		else
		{
			numberOfBatchSamples = remainingNumberOfSamples;
		}

		remainingNumberOfSamples -= numberOfBatchSamples;

		UxHwDoubleSampleBatch(tempUniformDistribution, uniformSampleArray, numberOfBatchSamples);

		for (uint64_t i = 0; i < numberOfBatchSamples; i++)
		{
			if (uniformSampleArray[i] < probabilityEvidence0GivenPhiPrior)
			{
				zeroEvidenceCount++;
			}
		}
	}

	zeroEvidenceProbability = (double) zeroEvidenceCount / numberOfEvidenceSamples;
	evidenceWeightedSamples[0].sample = 0.0;
	evidenceWeightedSamples[0].sampleWeight = zeroEvidenceProbability;
	evidenceWeightedSamples[1].sample = 1.0;
	evidenceWeightedSamples[1].sampleWeight = 1.0 - zeroEvidenceProbability;

	*evidenceDistribution = UxHwDoubleDistFromWeightedSamples(evidenceWeightedSamples, 2);

	return;
}


bool
runAQPEExperiment(
	double			initialPrior,
	CommandLineArguments *	arguments,
	size_t			experimentNo,
	size_t *		convergenceIterationCount,
	double *		estimatedPhi)
{
	double			phiPrior[kMaxNumberOfIterations + 1];
	double			evidenceDistribution;
	double			meanValue;
	double			standardDeviation;
	EvidenceModelParameters	evidenceModelParameters;
	bool			convergenceAchieved  = false;
	size_t			i;

	/*
	 *	AQPE experiment initializations
	 */
	phiPrior[0] = initialPrior;
	meanValue = UxHwDoubleNthMoment(phiPrior[0], 1);
	standardDeviation = sqrt(UxHwDoubleNthMoment(phiPrior[0], 2));

	if ((!arguments->outputPipelineMode) && (arguments->verbose))
	{
		printf("\nStarting AQPE Experiment #%zu:\n", experimentNo);
		printf("-------------------------------\n");
		printf("Iteration 0: Estimate Phi: %le\n", phiPrior[0]);
	}

	/*
	 *	Loop over Bayes-Laplace iterations
	 */
	for (i = 0; i < kMaxNumberOfIterations; i++)
	{
		evidenceModelParameters.currentM = calculateM(standardDeviation, arguments->alpha);
		evidenceModelParameters.currentTheta = calculateTheta(meanValue, standardDeviation);

		runQPECircuit(
			arguments->targetPhi,
			&evidenceDistribution,
			arguments->numberOfEvidenceSamplesPerIteration,
			evidenceModelParameters);
		phiPrior[i + 1] = UxHwDoubleBayesLaplace(
					&evidenceModel,
					(void *)(&evidenceModelParameters),
					phiPrior[i],
					evidenceDistribution,
					arguments->numberOfEvidenceSamplesPerIteration);

		if (isnan(phiPrior[i + 1]))
		{
			if ((!arguments->outputPipelineMode) && (arguments->verbose))
			{
				printf("\nWarning: Posterior is NAN! Please use a larger precision value (via -p option).\n");
			}

			break;
		}

		meanValue = UxHwDoubleNthMoment(phiPrior[i + 1], 1);
		standardDeviation = sqrt(UxHwDoubleNthMoment(phiPrior[i + 1], 2));

		if ((!arguments->outputPipelineMode) && (arguments->verbose))
		{
			printf("\nIteration %zu: Estimate Phi %le with mean value %le and standard deviation %le\n", i + 1, phiPrior[i + 1], meanValue, standardDeviation);
		}

		/*
		 *	If the standard deviation of posterior is smaller than precision, terminate.
		 */
		if (standardDeviation < arguments->precision)
		{
			convergenceAchieved = true;
			*estimatedPhi = phiPrior[i + 1];
			*convergenceIterationCount = i + 1;
			break;
		}
	}

	/*
	 *	Report the results of the current experiment.
	 */
	if ((!arguments->outputPipelineMode) && (arguments->verbose))
	{
		if (convergenceAchieved)
		{
			printf("\nAQPE Experiment #%zu: Successfully achieved precision in %zu iterative circuit mappings to quantum hardware! The final estimate is %le.\n", experimentNo, *convergenceIterationCount, *estimatedPhi);
		}
		else
		{
			printf("\nAQPE Experiment #%zu: Could not converge within the maximum allowed number of %d iterative circuit mappings to quantum hardware! The final estimate is %le.\n", experimentNo, kMaxNumberOfIterations, phiPrior[i]);
		}
	}

	return convergenceAchieved;
}

int
main(int argc, char *  argv[])
{
	CommandLineArguments	arguments = {
		.inputFilePath				= "input.csv",
		.outputFilePath				= "./sd0/aqpeOutput.csv",
		.outputPipelineMode			= false,
		.targetPhi				= M_PI / 2,
		.precision				= 1e-4,
		.alpha					= 0.5,
		.numberOfEvidenceSamplesPerIteration	= 0,
		.numberOfRepetitions			= 1,
		.verbose				= false,
		.fileSet				= false,
		.priorSet				= false,
		.priorInformation			= 0.0
	};
	double		initialPrior;
	double		estimatedPhi;
	size_t		convergenceIterationCount;
	double *	numberOfTotalIterationsArray;
	double *	distanceFromTargetArray;
	char   *	outputVariableNames[kNumberOfOutputDistributions] = {"numberOfTotalIterations", "distanceFromTarget"};
	double		outputVariables[kNumberOfOutputDistributions];
	size_t		wrongConvergenceCount = 0;
	size_t		convergenceCount = 0;
	double		xSigmaValue = 4.0;
	size_t		i;

	/*
	 *	Get command line arguments.
	 */
	if (getCommandLineArguments(argc, argv, &arguments))
	{
		return 1;
	}

	/*
	 *	Read input data.
	 */
	if (arguments.priorSet)
	{
		initialPrior = arguments.priorInformation;
	}
	else
	{
		if (readInputDistributionsFromCSV(arguments.inputFilePath, &initialPrior, 1))
		{
			return 1;
		}
	}

	/*
	 *	Allocate arrays.
	 */
	numberOfTotalIterationsArray = (double *) malloc(arguments.numberOfRepetitions * sizeof(double));
	if (numberOfTotalIterationsArray == NULL)
	{
		fprintf(stderr, "Allocation failed for 'numberOfTotalIterationsArray'. Exiting.\n");
		return 1;
	}

	distanceFromTargetArray = (double *) malloc(arguments.numberOfRepetitions * sizeof(double));
	if (distanceFromTargetArray == NULL)
	{
		fprintf(stderr, "Allocation failed for 'distanceFromTargetArray'. Exiting.\n");
		return 1;
	}

	/*
	 *	Loop over AQPE experiments
	 */
	for (i = 0; i < arguments.numberOfRepetitions; i++)
	{
		/*
		 *	Run experiment and count converging experiments.
		 */
		if (runAQPEExperiment(initialPrior, &arguments, i + 1, &convergenceIterationCount, &estimatedPhi))
		{
			/*
			 *	Recording output variables of interest.
			 */
			numberOfTotalIterationsArray[convergenceCount] = (double) convergenceIterationCount;
			distanceFromTargetArray[convergenceCount] = fabs(arguments.targetPhi - UxHwDoubleNthMoment(estimatedPhi, 1));

			/*
			 *	Counting wrong-converging experiments.
			 */
			if (distanceFromTargetArray[convergenceCount] > xSigmaValue * arguments.precision)
			{
				wrongConvergenceCount++;
			}

			convergenceCount++;
		}
	}

	/*
	 *	Report results across all experiments.
	 */
	if (convergenceCount == 0)
	{
		if (!arguments.outputPipelineMode)
		{
			printf("\nConvergence failed for all %zu AQPE experiments within the allowed maximum limit of %d iterative circuit mappings to quantum hardware!\n", arguments.numberOfRepetitions, kMaxNumberOfIterations);
		}
	}
	else
	{
		outputVariables[0] = UxHwDoubleDistFromSamples(numberOfTotalIterationsArray, convergenceCount);
		outputVariables[1] = UxHwDoubleDistFromSamples(distanceFromTargetArray, convergenceCount);

		if (!arguments.outputPipelineMode)
		{
			printf("\nConvergence achieved in %lf iterative circuit mappings to quantum hardware in %zu of %zu AQPE experiments and yielded a phase estimation error of %le.\n", outputVariables[0], convergenceCount, arguments.numberOfRepetitions, outputVariables[1]);
			printf("\nIn %zu out of %zu converging experiments, the phase estimation error was greater than %d times the input precision %le.\n", wrongConvergenceCount, convergenceCount, (int) xSigmaValue, xSigmaValue * arguments.precision);
		}
	}

	/*
	 *	Verbose mode reminder.
	 */
	if ((!arguments.outputPipelineMode) && (!arguments.verbose))
	{
		printf("\nTo print details of all experiments, please run in verbose mode using the '-v' command-line argument option.\n");
	}

	/*
	 *	Free allocated arrays.
	 */
	free(numberOfTotalIterationsArray);
	free(distanceFromTargetArray);

	/*
	 *	Write output data if there is data.
	 */
	if (convergenceCount > 0)
	{
		if (writeOutputDistributionsToCSV(arguments.outputFilePath, outputVariables, outputVariableNames, kNumberOfOutputDistributions))
		{
			return 1;
		}
	}

	return 0;
}
