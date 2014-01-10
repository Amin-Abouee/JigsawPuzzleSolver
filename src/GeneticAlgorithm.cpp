#include "GeneticAlgorithm.hpp"
#include "Choromosome.hpp"
#include "Piece.hpp"
#include <vector>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <algorithm>
#include <assert.h>
#include <random>

GeneticAlgorithm::GeneticAlgorithm(cv::Mat image, int size):image(image), patchSize(size,size)
{
    numPiecesRow = image.rows / size;
    numPiecesCol = image.cols / size;
    numPieces = (image.rows / size) * (image.cols / size);
    totalFitness = 0;
    pieces.reserve(numPieces);
    generator.seed(rd());
}

GeneticAlgorithm::~GeneticAlgorithm()
{

}

void GeneticAlgorithm::generatePopulation(int numChoromosomes)
{
    population.reserve(numChoromosomes);
    std::vector <int> randVec (numPieces);
    for(size_t j=0; j<numPieces; j++)
        randVec[j]=j;

    for(size_t i=0; i<numChoromosomes; i++)
    {
        Choromosome temp(numPiecesCol, numPiecesRow);
        temp.generateChoromosome(randVec);
        population.push_back(temp);
    }
}

void GeneticAlgorithm::generatePieces()
{
    for(size_t i=0; i< numPiecesRow; i++)
        for(size_t j=0; j<numPiecesCol; j++)
        {
            cv::Mat patchImage = splitImage(i,j);

            assert(patchImage.rows == patchSize.height);
            assert(patchImage.cols == patchSize.width);
            int id = i * numPiecesRow + j;
            pieces.push_back(Piece(id, patchImage));
        }
}

void GeneticAlgorithm::initiatePieces()
{
    for(size_t i=0; i<pieces.size(); i++)
        for(size_t j=0; j<pieces.size(); j++)
        {
            if(i == j)
                continue;
            pieces[i].calculatePairwiseCompatibility(pieces[j]);
        }

    for(auto & piece : pieces)
    {
        piece.sortDissimilarityValues();
        piece.setBestBuddies();
    }
}

void GeneticAlgorithm::evaluateAllChoromosoms()
{
    // use inside the selection method
    totalFitness = 0;
    for(auto & choromosome : population)
    {
        choromosome.calculateFitness(pieces);
        totalFitness += choromosome.getFitness();
    }
}

const Choromosome & GeneticAlgorithm::selectionChromosome()
{
    std::uniform_real_distribution <double> distribution(0.0,1.0);
    double randomFitness = distribution(generator);

    double sum = 0;
    // check -> population sorted here ????
    for(const auto & choromosome : population)
    {
        sum += (choromosome.getFitness() / totalFitness);
        if(sum >= randomFitness)
            return choromosome;
    }
    std::cerr << "choromosome not selected!!" <<std::endl;
}

void GeneticAlgorithm::selectElitism(int numElitism)
{
    assert(numElitism < population.size());

    for(int i=0; i<numElitism; i++)
        newPopulation.push_back(population[i]);
}

void GeneticAlgorithm::mutation()
{
    float numMutation = numPieces * 0.05;
    int populationSize = population.size();
    std::uniform_int_distribution<int> distrioRow(0,numPiecesRow-1);
    std::uniform_int_distribution<int> distrioCol(0,numPiecesCol-1);

    for(size_t c =0; c < populationSize; c++)
    {
        Choromosome & choromosome = population[c];
        int numMutationForEachChoro = c * (numMutation / populationSize);
//        std::cout << "num for choro "<< c <<" :"<< numMutationForEachChoro << "with fitness: "<< choromosome.getFitness() << std::endl;
        for(int i=0; i<numMutationForEachChoro; i++)
        {
            int randomRowInx1 = distrioRow(generator);
            int randomRowInx2 = distrioRow(generator);
            int randomColInx1 = distrioCol(generator);
            int randomColInx2 = distrioCol(generator);

            assert (randomRowInx1 != randomRowInx2 && randomColInx1 != randomColInx2);
           choromosome.swapTwoPieces(randomRowInx1, randomColInx1, randomRowInx2, randomColInx2);
        }
    }
}

void GeneticAlgorithm::crossOver(const Choromosome &parent1, const Choromosome &parent2)
{
    std::uniform_int_distribution<int> distribution(0,numPieces-1);
    int seedInx = distribution(generator);

    Choromosome offSpring(numPiecesCol, numPiecesRow);
    // TODO assignPiece for one sample;
    offSpring.assignPiece(numPiecesRow/2, numPiecesCol/2, seedInx);

    while(offSpring.getOccupiedPositions() < numPieces)
    {
        std::vector <SpatialRelation> freeBounderiesPositions;

        offSpring.getFreeBoundries(freeBounderiesPositions);
        //

        bool resultParent = setNeighbourByParents(offSpring, parent1, parent2, freeBounderiesPositions);
        if(resultParent)
        {
            //            std::cout << "parent!!"<<std::endl;
            continue;
        }

        bool resultBesBuddy = setNeighbourByBestBuddy(offSpring, parent1, parent2, freeBounderiesPositions);
        if(resultBesBuddy)
        {
            //            std::cout << "bestBuddy!!"<<std::endl;
            continue;
        }

        std::uniform_int_distribution<int> distribution1(0,freeBounderiesPositions.size()-1);
        int randomPieceId  = distribution1(generator);

        setNeighbourByBestMatch(offSpring, freeBounderiesPositions[randomPieceId]);
        //        std::cout << "bestmatch!!"<<std::endl;
    }
    newPopulation.push_back(offSpring);
}

cv::Mat GeneticAlgorithm::splitImage(int rowPiece, int colPiece) const
{
    cv::Rect ROIRect(colPiece * patchSize.width, rowPiece * patchSize.height, patchSize.width, patchSize.height);
    cv::Mat ROI = image(ROIRect);
    return ROI;
}

int GeneticAlgorithm::findNeighbourByParents(const Choromosome & parent1, const Choromosome & parent2, const SpatialRelation & currentBoundary)
{
    int neighbourParent1 = parent1.getNeighbour(currentBoundary);
    int neighbourParent2 = parent2.getNeighbour(currentBoundary);

    if(neighbourParent1 != -1 && neighbourParent2 != -1)
    {
        if(neighbourParent1 == neighbourParent2)
        {
            return neighbourParent1;
        }
    }

    return -1;
}

bool GeneticAlgorithm::setNeighbourByParents(Choromosome &offSpring,
                                             const Choromosome &parent1,
                                             const Choromosome &parent2,
                                             const std::vector <SpatialRelation> & freeBounderiesPositions)
{
    for(size_t i=0; i<freeBounderiesPositions.size(); i++)
    {
        SpatialRelation currentBoundary = freeBounderiesPositions[i];
        int pieceId = findNeighbourByParents(parent1, parent2, currentBoundary);
        if (pieceId != -1 && offSpring.checkPieceAvailability(pieceId))
        {
            offSpring.assignPiece(currentBoundary, pieceId);
            return true;
        }
    }
    return false;
}


int GeneticAlgorithm::findNeighbourByBestBuddy(const Choromosome &parent1,
                                               const Choromosome &parent2,
                                               const SpatialRelation &currentBoundary)
{
    int neighbourParent1 = parent1.getNeighbour(currentBoundary);
    int neighbourParent2 = parent2.getNeighbour(currentBoundary);

    int currentPieceId = currentBoundary.pieceIndex;

    if (pieces[currentPieceId].isBestBuddy(pieces[neighbourParent1], currentBoundary.direction))
    {
        return neighbourParent1;
    }

    else if (pieces[currentPieceId].isBestBuddy(pieces[neighbourParent2], currentBoundary.direction))
    {
        return neighbourParent2;
    }

    return -1;
}

bool GeneticAlgorithm::setNeighbourByBestBuddy(Choromosome &offSpring,
                                               const Choromosome &parent1,
                                               const Choromosome &parent2,
                                               const std::vector<SpatialRelation> &freeBounderiesPositions)
{
    for(size_t i=0; i<freeBounderiesPositions.size(); i++)
    {
        SpatialRelation currentBoundary = freeBounderiesPositions[i];
        int pieceId = findNeighbourByBestBuddy(parent1, parent2, currentBoundary);
        if (pieceId != -1 && offSpring.checkPieceAvailability(pieceId))
        {
            offSpring.assignPiece(currentBoundary, pieceId);
            return true;
        }
    }
    return false;
}

void GeneticAlgorithm::setNeighbourByBestMatch(Choromosome & offSpring,
                                               const SpatialRelation &currentBoundary)
{
    int neighbourId = pieces[currentBoundary.pieceIndex].getBestMatch(offSpring.availabalePieces, currentBoundary.direction);
    assert (neighbourId != -1);
    offSpring.assignPiece(currentBoundary, neighbourId);
}

void GeneticAlgorithm::copyNewPopulationToPopulation()
{
    population = newPopulation;
    newPopulation.clear();
}

Choromosome GeneticAlgorithm::getBestChromosome()
{
    std::sort(population.begin(), population.end());
    return population[0];
}

void GeneticAlgorithm::printPopulation()
{
    for(const auto & choromosome : population)
        choromosome.printChoromosome();
}
