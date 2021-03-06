#ifndef GENETICALGORITHM_HPP
#define GENETICALGORITHM_HPP

#include <iostream>
#include <opencv2/opencv.hpp>
#include <random>
#include "Choromosome.hpp"
#include "Piece.hpp"
#include "SpatialRelation.hpp"

class GeneticAlgorithm
{
public:

    //C'tor
    GeneticAlgorithm(cv::Mat image, int size);

    //D'tor
    virtual ~GeneticAlgorithm();

    const Choromosome & selectionChromosome();

    void selectElitism(int numElitism);

    void crossOver(const Choromosome & parent1, const Choromosome & parent2);

    void mutation(Choromosome & offSpring,
                  int numAvailable,
                  const SpatialRelation &currentBoundary);

    cv::Mat splitImage(int rowPiece, int colPiece) const ;

    void generatePopulation(int numChoromosomes);

    void generatePieces();

    void evaluateAllChoromosoms();

    void initiatePieces();

    void copyNewPopulationToPopulation();

    void printPopulation();

    Choromosome getBestChromosome();

    int findNeighbourByParents(const Choromosome & parent1,
                               const Choromosome & parent2,
                               const SpatialRelation & currentBoundary);

    bool setNeighbourByParents (Choromosome & offSpring,
                                const Choromosome & parent1,
                                const Choromosome & parent2,
                                const std::vector <SpatialRelation> & freeBounderiesPositions);

    int findNeighbourByBestBuddy(const Choromosome & parent1,
                                 const Choromosome & parent2,
                                 const SpatialRelation & currentBoundary);

    bool setNeighbourByBestBuddy(Choromosome & offSpring,
                                 const Choromosome & parent1,
                                 const Choromosome & parent2,
                                 const std::vector <SpatialRelation> & freeBounderiesPositions);

    void setNeighbourByBestMatch (Choromosome & offSpring,
                                  const SpatialRelation &currentBoundary);


private:

    std::vector <Choromosome> population;

    std::vector <Choromosome> newPopulation;

    std::vector <Piece> pieces;

    cv::Mat image;

    cv::Size patchSize;

    int numPieces;

    double totalFitness;

    int numPiecesRow;

    int numPiecesCol;

    std::mt19937_64 generator;

    std::random_device randomSeed;
};

#endif /* GENETICALGORITHM_HPP */
