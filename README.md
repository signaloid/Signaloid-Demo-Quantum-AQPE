[<img src="https://assets.signaloid.io/add-to-signaloid-cloud-logo-dark-v6.png#gh-dark-mode-only" alt="[Add to signaloid.io]" height="30">](https://signaloid.io/repositories?connect=https://github.com/signaloid/Signaloid-Demo-Quantum-AQPE#gh-dark-mode-only)
[<img src="https://assets.signaloid.io/add-to-signaloid-cloud-logo-light-v6.png#gh-light-mode-only" alt="[Add to signaloid.io]" height="30">](https://signaloid.io/repositories?connect=https://github.com/signaloid/Signaloid-Demo-Quantum-AQPE#gh-light-mode-only)

# Accelerated Quantum Phase Estimation (AQPE) Exploiting the Signaloid Compute Engine
This repository contains an implementation of the classical part of the Accelerated Quantum Phase Estimation (AQPE) subroutine of the Accelerated Variational Quantum Eigensolver (AVQE) based on the work by Cruise et al. [^0], which takes advantage of the capabilities of Signaloid's compute engine to achieve faster convergence. The implementation is a variation of the AVQE application evaluated by Tsoutsouras et al. [^1]. For phase estimation, the implementation uses the Signaloid compute engine's Bayes-Laplace operation to implement Bayesian inference. For comparison, the repository [Signaloid-Demo-Quantum-AQPE-NoUx](https://github.com/signaloid/Signaloid-Demo-Quantum-AQPE-NoUx) contains an implementation of AQPE using an approximation to Bayesian inference called Reduction Filtering Phase Estimation (RFPE).

## The AQPE Algorithm
The AQPE algorithm is a method to estimate the eigenphase $\phi$ corresponding to the eigenvector $\ket{\phi}$ of a single-qubit quantum gate represented by the unitary matrix $U$, that is, $U\ket{\phi} = e^{i\phi}\ket{\phi}$. AQPE uses the quantum circuit below to obtain the evidence for estimating the value of $\phi$.

<img width="800" alt="image" src="https://user-images.githubusercontent.com/115564080/235872639-cb6866b2-cfcb-421f-b2cf-538110ca43fe.png">

AVQE is a generalization of the standard Variational Quantum Eigensolver (VQE) which replaces the Quantum Expectation Estimation (QEE) subroutine with the AQPE subroutine and is a bridge between VQE and the classical QPE algorithm by Kitaev et al. [^2]. AVQE introduces a parameter $\alpha \in [0,1]$ that enables a tradeoff between the required quantum circuit depth $D$ and the number of required quantum circuit measurements $N$ to acheive a specified precision $p$ in estimating the value of the phase $\phi$.

| Algorithm | Quantum Circuit Depth, $D$ | Number of Quantum Circuit Measurements, $N$ |
| ------ | --- | --- |
| AVQE | $O(p^{-\alpha})$ | $O(p^{{-2(1 - \alpha)}})$ for $\alpha \in [0,1)$, $O(\mathrm{log}(p^{-1}))$ for $\alpha = 1$|
| VQE | $O(1)$ | $O(p^{-2})$ |
| Kitaev's QPE| $O(p^{-1})$ | $O(1)$ |

The table above shows the requirements for the quantum circuit depth $D$ and the number of quantum circuit measurements $N$ of each algorithm (AVQE, VQE, and Kitaev's QPE). For the quantum circuit depth, AVQE requires that $D = O(p^{-\alpha})$ for $\alpha \in [0,1]$, and for the number of quantum circuit measurements, AVQE requires that $N = O(p^{{-2(1 - \alpha)}})$ for $\alpha \in [0,1)$ and $N = O(\mathrm{log}(p^{-1}))$ for $\alpha = 1$. When $\alpha = 0$, AVQE requirements exactly match that of VQE which has $D = O(1)$ and $N = O(p^{-2})$. On the other hand, when $\alpha = 1$, AVQE requirements match (upto a logarithmic factor for $N$) that of Kitaev's QPE which has $D = O(p^{-1})$ and $N = O(1)$.

With the contemporary NISQ-era quantum computers being capable of maintaining qbit coherence for quantum circuit depths of only few hundreds, implementing Kitaev's QPE approach is out of question for low values of $p$ (e.g., $p \leq 10^{-3}$ for which $D \geq O(10^3) $). On the other hand, the high number of required quantum circuit measurements for VQE renders it prohibitive for low values of $p$ (e.g., $p \leq 10^{-3}$ for which $N \geq O(10^6) $). AVQE allows one to achieve a trade-off between $D$ and $N$, where one would choose $\alpha > 0$ for a given maximum depth $D_{\mathrm{max}}$ that an available NISQ computing machine can sustain and would require less number of quantum circuit measurements compared to VQE.

## An interesting use case for NISQ era
After clicking on the "add to signaloid.io" button at the top of this README, you will be connected to the Repositories Tab on the Signaloid Cloud Developer Platform. Next, click on the <img width="45" alt="Screenshot 2023-06-29 at 22 55 31" src="https://github.com/signaloid/Signaloid-Demo-Quantum-AQPE-NoUx/assets/86417/6a076901-ae9b-4933-bf89-d3120baa29f8"> button to set the command-line arguments to `-i -o -p 1e-4 -a 0.5`. This sets $\alpha = 0.5$ and the estimation precision to $10^{-4}$. For these values, the required quantum circuit depth is $D = 100$ and the required number of shots or measurements per quantum circuit mapped to a quantum computer is $N = 39996$. This depth is achievable by NISQ-era quantum computers.

## Signaloid Highlights
Compared with the [RFPE approach](https://github.com/signaloid/Signaloid-Demo-Quantum-AQPE-NoUx), the implementation of AQPE in this repository takes advantage of the Bayes-Laplace operation supported by the Signaloid compute engine and exposed thorugh the `UxHw` API, to achieve:
1. Faster execution times.
2. More robust convergence statistics.
3. Fewer total required number of iterative circuit mappings to quantum hardware to achieve convergence.
4. Fewer total required number of quantum circuit measurements to achieve convergence.
5. Easier access to Bayesian inference without the need to worry about implementing an efficient approximation of Bayesian inference as in the case of RFPE.

The table below compares the results of the Signaloid implementation against the RFPE method in $1000$ repeated AQPE experiments (using the `-r 1000` command-line argument) for $\alpha = 0.5$ and the estimation precision $p = 10^{-4}$:
| Performance metric | Signaloid solution (C0pro-S+) [^3] | [RFPE method](https://github.com/signaloid/Signaloid-Demo-Quantum-AQPE-NoUx) | Signaloid improvement percentages |
| -- | :--: | :--: | :--: |
| **Faster execution times:**<br>Elapsed user time for $1000$ AQPE experiments [^4] | $3.657$ seconds | $4.057$ seconds | 9.86% faster classical component runtime |
| **More robust convergence statistics:**<br>Number of successful convergences (within $4p$ of target) | $992$ (of $1000$ repeated experiments) | $751$  (of $1000$ repeated experiments) | 32.09% more robust convergence statistics|
| **Fewer total required number of circuit mappings to quantum hardware to achieve convergence:**<br>Average total required number of quantum circuit mappings | $4.188$ quantum circuit mappings | $7.599$ quantum circuit mappings | 44.89% better hardware usage efficiency |
| **Fewer total required number of quantum circuit measurements to achieve convergence:**<br>Average total required number of shots | $167503$ shots | $303930$ shots | 44.89% better hardware usage efficiency |
| **Easier access to Bayesian inference:**<br>Required number of lines of code for Bayesian inference | $14$ lines of code [^5] | $63$ lines of code | 77.78% shorter code |

## Usage
```
[-s <K> <sample1> <sample2> ... <sampleK> <sampleWeight1> <sampleWeight2> ... <sampleWeightK>] K in [1, 100000]
[-i [path_to_input_csv_file : str] (Default: 'input.csv')]
[-o [path_to_output_csv_file : str] (Default: './sd0/aqpeOutput.csv')]
[-t <target_phase : double in [-pi, pi]>] (Default: pi / 2)
[-p <precision_in_phase_estimation : double in (0, inf)>] (Default: 1e-4)
[-a <alpha : double in [0,1]>]  (Default: 0.5)
[-n <number_of_evidence_samples_per_iteration : int in [1, inf)>] (Default: 1 / precision^{alpha})
[-r <number_of_repetitions : size_t in (0, inf)>] (Default: 1)
[-v] (Verbose mode: Prints details of each repeated AQPE experiment to stdout.)
[-h] (Display this help message.)
```

## Repository Tree Structure
```
.
├── LICENSE
├── README.md
├── inputs
│   └── input.csv
└── src
    ├── README.MD
    ├── main.c
    ├── utilities.c
    └── utilities.h
```

## References
[^0]: J. R. Cruise, N. I. Gillespie, and B. Reid: Practical Quantum Computing: The value of local computation. arXiv:2009.08513 [quant-ph], Sep. 2020. Available Online: http://arxiv.org/abs/2009.08513.

[^1]: Vasileios Tsoutsouras, Orestis Kaparounakis, Bilgesu Arif Bilgin, Chatura Samarakoon, James Timothy Meech, Jan Heck, Phillip Stanley-Marbell: The Laplace Microarchitecture for Tracking Data Uncertainty and Its Implementation in a RISC-V Processor. MICRO 2021: 1254-1269.

[^2]: A. Y. Kitaev, A. Shen, and M. N. Vyalyi: Classical and Quantum Computation. American Mathematical Society, 2002.

[^3]: C0pro-S+ is a core from the C0pro family and has the same configuration parameters as the standard C0-S+ core. In contrast to standard C0 cores, C0pro cores do not suffer from the virtualization overhead caused by our hardware emulator. C0pro cores are in limited private beta: we are indeed making it available for customers who get dedicated servers.

[^4]: We evaluated the performance for the Signaloid solution versus RFPE running on a 16-core Intel Xeon Platinum 8275CL CPU running at 3.00GHz.

[^5]: In our implementation, there are an additional 22 lines of safety-checking code to check for memory allocation bounds.
