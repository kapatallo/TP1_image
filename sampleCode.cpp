#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <iostream>
#include <numeric>
#include <vector>
#include <string>
#include <exception>
#include <stdexcept>
#include <ctime>
#include <cmath>
#include <queue>

using namespace cv;
using namespace std;

class PixelInfo {
    int category, posX, posY, intensity;

public:
    PixelInfo(int category, int posX, int posY, int intensity);
    int getPositionX() const;
    int getPositionY() const;
    int getCategory() const;
    int getIntensity() const;
    void setCategory(int category);
    void display();
};

PixelInfo::PixelInfo(int category, int posX, int posY, int intensity) : category(category), posX(posX), posY(posY), intensity(intensity) {}

int PixelInfo::getPositionX() const { return posX; }
int PixelInfo::getPositionY() const { return posY; }
int PixelInfo::getCategory() const { return category; }
int PixelInfo::getIntensity() const { return intensity; }
void PixelInfo::setCategory(int category) { this->category = category; }
void PixelInfo::display() { cout << "( Category: " << category << " , X: " << posX << " , Y: " << posY << " , Intensity: " << intensity << " )" << endl; }

class ImageHandler {
public:
    static Mat loadImage(const string& filepath, ImreadModes mode);
    static Mat resizeImage(const Mat& img, double width, double height);
    static void showImage(const Mat& img, const string& windowName);
};

Mat ImageHandler::loadImage(const string& filepath, ImreadModes mode) {
    string fullPath;
    try {
        fullPath = samples::findFile(filepath);
    }
    catch (const exception& e) {
        cerr << e.what();
        exit(EXIT_FAILURE);
    }

    Mat img = imread(fullPath, mode);
    if (img.empty()) {
        throw invalid_argument("Invalid file path provided!");
        exit(EXIT_FAILURE);
    }
    return img;
}


Mat ImageHandler::resizeImage(const Mat& img, double width, double height) {
    Mat resizedImg;
    resize(img, resizedImg, Size(width, height));
    return resizedImg;
}

void ImageHandler::showImage(const Mat& img, const string& windowName) {
    imshow(windowName, img);
}

class RegionGrower {
    Mat originalImage, regionMap;
    queue<PixelInfo> pixelQueue;
    Mat borderEffectiveness, borderSimilarity;

    vector<PixelInfo> findNeighbors(const PixelInfo& pixel);
    bool isValidCoordinate(int x, int y) const;
    void addNeighbor(int x, int y, vector<PixelInfo>& neighbors);
    bool growthCriterion(const PixelInfo& current, const PixelInfo& neighbor);
    bool mergeCriterion(int current, int neighbor);

public:
    RegionGrower(const Mat& img);
    Mat execute(int seedCount);
    Mat drawRegionBorders(const Mat& regionMap, int borderThickness);

};

RegionGrower::RegionGrower(const Mat& img) : pixelQueue(), regionMap(Mat::zeros(img.rows, img.cols, CV_8UC1)) {
    img.convertTo(originalImage, CV_8UC1);
}

vector<PixelInfo> RegionGrower::findNeighbors(const PixelInfo& pixel) {
    vector<PixelInfo> neighbors;
    addNeighbor(pixel.getPositionX() - 1, pixel.getPositionY(), neighbors);
    addNeighbor(pixel.getPositionX() + 1, pixel.getPositionY(), neighbors);
    addNeighbor(pixel.getPositionX(), pixel.getPositionY() - 1, neighbors);
    addNeighbor(pixel.getPositionX(), pixel.getPositionY() + 1, neighbors);

    return neighbors;
}

bool RegionGrower::isValidCoordinate(int x, int y) const {
    return (y >= 0 && y < originalImage.rows) && (x >= 0 && x < originalImage.cols);
}

void RegionGrower::addNeighbor(int x, int y, vector<PixelInfo>& neighbors) {
    if (isValidCoordinate(x, y)) {
        neighbors.emplace_back(regionMap.at<uchar>(y, x), x, y, originalImage.at<uchar>(y, x));
    }
}

bool RegionGrower::growthCriterion(const PixelInfo& current, const PixelInfo& neighbor) {
    return abs(current.getIntensity() - neighbor.getIntensity()) <= 3;
}

bool RegionGrower::mergeCriterion(int current, int neighbor) {
    return abs(current - neighbor) <= 10;
}

Mat RegionGrower::drawRegionBorders(const Mat& colorRegionMap, int borderThickness) {
    Mat borders = Mat::zeros(colorRegionMap.size(), CV_8UC1);

    for (int y = 0; y < colorRegionMap.rows; ++y) {
        for (int x = 0; x < colorRegionMap.cols; ++x) {
            // Détermine si le pixel actuel est à la frontière
            bool isBorder = false;
            Vec3b color = colorRegionMap.at<Vec3b>(y, x);

            if (x > 0 && color != colorRegionMap.at<Vec3b>(y, x - 1)) isBorder = true;
            if (x < colorRegionMap.cols - 1 && color != colorRegionMap.at<Vec3b>(y, x + 1)) isBorder = true;
            if (y > 0 && color != colorRegionMap.at<Vec3b>(y - 1, x)) isBorder = true;
            if (y < colorRegionMap.rows - 1 && color != colorRegionMap.at<Vec3b>(y + 1, x)) isBorder = true;

            // Si le pixel est à la frontière, le marquer
            if (isBorder) {
                borders.at<uchar>(y, x) = 255;
            }
        }
    }

    // Épaissir les frontières si nécessaire
    if (borderThickness > 1) {
        Mat kernel = getStructuringElement(MORPH_RECT, Size(borderThickness, borderThickness));
        dilate(borders, borders, kernel);
    }

    return borders;
}





Mat RegionGrower::execute(int seedCount) {
    // Initialisation des matrices pour l'analyse des frontières
    borderEffectiveness = Mat::zeros(seedCount + 1, seedCount + 1, CV_32S);
    borderSimilarity = Mat::zeros(seedCount + 1, seedCount + 1, CV_64F);

    // Initialisation des germes
    int gridRows = ceil(sqrt(seedCount));
    int gridCols = ceil(sqrt(seedCount));
    int dx = originalImage.cols / gridCols;
    int dy = originalImage.rows / gridRows;
    int seedIndex = 1;

    for (int i = 0; i < gridRows; ++i) {
        for (int j = 0; j < gridCols; ++j) {
            if (seedIndex > seedCount) break;

            int x = j * dx + dx / 2;
            int y = i * dy + dy / 2;

            if (x >= originalImage.cols) x = originalImage.cols - 1;
            if (y >= originalImage.rows) y = originalImage.rows - 1;

            if (regionMap.at<uchar>(y, x) == 0) {
                pixelQueue.push(PixelInfo(seedIndex, x, y, originalImage.at<uchar>(y, x)));
                regionMap.at<uchar>(y, x) = static_cast<uchar>(seedIndex);
                seedIndex++;
            }
        }
    }

    // Croissance des régions
    while (!pixelQueue.empty()) {
        PixelInfo current = pixelQueue.front();
        pixelQueue.pop();

        vector<PixelInfo> neighbors = findNeighbors(current);
        for (auto& neighbor : neighbors) {
            if (growthCriterion(current, neighbor) && regionMap.at<uchar>(neighbor.getPositionY(), neighbor.getPositionX()) == 0) {
                regionMap.at<uchar>(neighbor.getPositionY(), neighbor.getPositionX()) = static_cast<uchar>(current.getCategory());
                neighbor.setCategory(current.getCategory());
                pixelQueue.push(neighbor);
            }
        }
    }

    // Analyse des frontières entre les régions
    for (int i = 0; i < originalImage.rows; ++i) {
        for (int j = 0; j < originalImage.cols; ++j) {
            if (j < originalImage.cols - 1) {
                int currentRegion = regionMap.at<uchar>(i, j);
                int neighborRegion = regionMap.at<uchar>(i, j + 1);
                if (currentRegion != neighborRegion) {
                    borderEffectiveness.at<int>(currentRegion, neighborRegion)++;
                    if (mergeCriterion(originalImage.at<uchar>(i, j), originalImage.at<uchar>(i, j + 1))) {
                        borderSimilarity.at<double>(currentRegion, neighborRegion)++;
                    }
                }
            }

            if (i < originalImage.rows - 1) {
                int currentRegion = regionMap.at<uchar>(i, j);
                int neighborRegion = regionMap.at<uchar>(i + 1, j);
                if (currentRegion != neighborRegion) {
                    borderEffectiveness.at<int>(currentRegion, neighborRegion)++;
                    if (mergeCriterion(originalImage.at<uchar>(i, j), originalImage.at<uchar>(i + 1, j))) {
                        borderSimilarity.at<double>(currentRegion, neighborRegion)++;
                    }
                }
            }
        }
    }

    // Fusion des régions similaires
    vector<int> regionMapping(seedCount + 1);
    iota(regionMapping.begin(), regionMapping.end(), 0);

    for (int i = 1; i <= seedCount; ++i) {
        for (int j = i; j <= seedCount; ++j) {
            if (borderEffectiveness.at<int>(i, j) > 0) {
                borderSimilarity.at<double>(i, j) /= borderEffectiveness.at<int>(i, j);
                if (borderSimilarity.at<double>(i, j) > 0.5) {
                    regionMapping[j] = regionMapping[i];
                }
            }
        }
    }

    for (int i = 0; i < regionMap.rows; ++i) {
        for (int j = 0; j < regionMap.cols; ++j) {
            regionMap.at<uchar>(i, j) = static_cast<uchar>(regionMapping[regionMap.at<uchar>(i, j)]);
        }
    }

    // Générer des couleurs aléatoires pour les régions
    vector<Vec3b> colors(seedCount + 1);
    for (int i = 1; i <= seedCount; ++i) {
        colors[i] = Vec3b(rand() % 256, rand() % 256, rand() % 256);
    }

    // Appliquer les couleurs aux régions
    Mat colorRegionMap = Mat::zeros(regionMap.size(), CV_8UC3);
    for (int i = 0; i < regionMap.rows; ++i) {
        for (int j = 0; j < regionMap.cols; ++j) {
            int regionId = regionMap.at<uchar>(i, j);
            colorRegionMap.at<Vec3b>(i, j) = colors[regionId];
        }
    }

    return colorRegionMap;
}


Mat filterNoise(const Mat& inputImage, int kernelSize) {
    if (kernelSize % 2 == 0) {
        return inputImage.clone();
    }

    int pad = kernelSize / 2;
    Mat outputImage = inputImage.clone();

    // Parcourir chaque pixel de l'image (sauf les bords)
    for (int i = pad; i < inputImage.rows - pad; i++) {
        for (int j = pad; j < inputImage.cols - pad; j++) {
            vector<int> neighbors;

            // Parcourir chaque voisin du pixel
            for (int k = -pad; k <= pad; k++) {
                for (int l = -pad; l <= pad; l++) {
                    int pixelValue = inputImage.at<uchar>(i + k, j + l);
                    neighbors.push_back(pixelValue);
                }
            }

            // Trier et prendre la valeur médiane
            sort(neighbors.begin(), neighbors.end());
            int medianValue = neighbors[neighbors.size() / 2];
            outputImage.at<uchar>(i, j) = medianValue;
        }
    }

    return outputImage;
}


void processImage(const string& path) {
    Mat img = ImageHandler::loadImage(path, IMREAD_GRAYSCALE);
    img = ImageHandler::resizeImage(img, 512, 512);
    int kernelSize = 1;
    img = filterNoise(img, kernelSize);
    ImageHandler::showImage(img, "Original Image");
    RegionGrower grower(img);

    Mat regionColorMap = grower.execute(200);
    ImageHandler::showImage(regionColorMap, "Region Grown Image");
    Mat regionBorders = grower.drawRegionBorders(regionColorMap, 1); // 1 est l'épaisseur des frontières
    ImageHandler::showImage(regionBorders, "Region Borders");
}

int main() {
    processImage("image1.jpg");
    waitKey(0);
    processImage("image2.jpg");
    waitKey(0);
    return 0;
}

 