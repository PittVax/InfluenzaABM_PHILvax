# Public Health Influenza Laboratory (PHIL)

## Synopsis

PHIL uses agent-based modeling based on census-based synthetic populations that capture the demographic and geographic distributions of the population, as well as detailed household, school, and workplace social networks. Multiple circulating and evolving strains can be simulated. Mitigation strategies in the framework include vaccination, anti-viral drugs, and school closure policies. PHIL supports models of health behavior change to facilitate the study of critical personal health behaviors such as vaccine acceptance, personal hygiene and spontaneous social distancing.

* **Realistic synthetic populations based on the US Census Bureau’s Public Use Microdata files (PUMs) and Census aggregated data.** Each agent has a set of socio-demographic characteristics and daily behaviors that include age, sex, employment status, occupation, and household location and membership. Each household, school, and workplace is mapped to a specific geographic location, reflecting the actual spatial distribution of the area and the distance travelled by individuals to work or to school. The synthetic population was developed by RTI International as part of the MIDAS project and is freely available.

* **Specification of agent health status and social networks.** Each agent in PHIL has associated health information (e.g., current health status, date of infection, level of symptoms, infectiousness, and susceptibility). During each simulated day, agents interact with the other agents who share the same social activity locations (i.e., children interact with other children in the same classroom at school).  If an infectious agent interacts with a susceptible agent, there is a possibility transmission of disease. PHIL simulates the population of agents during a period of time, usually several months or years, and tracks the spread of disease among the population.

* **Specification of agent health behavior and decision rules.** Agents in PHIL may exhibit a number of health-related behaviors involving individual health decisions, such as staying home when sick, accepting a vaccine or taking an anti-viral drug. The PHIL platform is designed to accommodate research on a range of theoretical models about health behavior and supports a variety of strategies to determine an agent’s willingness to adopt a behavior.

* **Scalable.** PHIL can run on a variety of computer platforms from laptops to supercomputers, depending on the size of the population being simulated.  

## Code Example

`phil` takes as a single argument a 'params' file.  See the `input_files` directory for examples.

## Motivation

Our agent-based influenza modeling framework, PHIL, has been developed during our R01 and is a highly modified version of the open source agent-based simulation platform called FRED (A Framework for Reconstructing Epidemic Dynamics) in collaboration with the Pittsburgh Supercomputing Center and the School of Computer Science at Carnegie Mellon University. Development of both PHIL and FRED were developed as part of the NIH funded MIDAS program. PHIL enables us to simulate the daily activities and interactions of millions of simulated individuals during an epidemic and to measure the effects of a wide range of intervention strategies for infectious disease. The MIDAS group has used such population-based computer simulations to evaluate potential responses to influenza pandemics, including vaccination policies (Lee et al, 2010acd, 2011), school closure (Lee et al, 2010b; Brown et al, 2011), and the effects of commuting modalities (Cooley et al, 2011). Key features of FRED include: Dr. Brown, Co-I, is well versed in the utilization of PHIL for modeling simulations.

## Installation

Compilation should be straightforward on both linux and OSX platforms.  In order to take advantage of multithreading through OpenMP, the `gcc` compiler should be used.

```bash
cd src
make
cd ..
make
```

## License

Both `phil` and `FRED` (on which `phil` is based), are distributed under the "BSD Simplified" open source license.
Code changes prior to the commit 895218b6e8a788ecf959ba535bf580bc240262be are copyright of the University of Pittsburgh.
All subsequent changes are copyright of Carnegie Mellon University.
