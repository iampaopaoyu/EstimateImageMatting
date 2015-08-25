#include "inputassembler.h"
#include "matrixd.h"
#include "io.h"
#include <set>
#include <stdexcept>

namespace anima
{
    namespace ia
    {
        cv::Mat InputAssembler::loadRgbMatFromFile(const char* path)
        {
            cv::Mat mat;
            mat = cv::imread(path);
            if(mat.data==nullptr)
                throw std::runtime_error("Could not load image data.");
            mat.convertTo(mat, CV_8UC3);

            return mat;
        }

        InputAssembler::InputAssembler(InputAssemblerDescriptor& desc)
        {
            START_TIMER(ProcessingInput);
            //Convert the input into 3 component float mat.
            if(desc.source == nullptr)
                throw std::runtime_error("Null source mat");

            double alpha;
            switch(desc.source->type())
            {
            case CV_8UC3:
                alpha = 1/255.f;
                break;
            case CV_16UC3:
                alpha = 1/65535.f;
                break;
            case CV_32FC3:
                alpha = 1.f;
                break;
            default:
                throw std::runtime_error("Unknown format in input assember at line " + ToString(__LINE__));
            }

            desc.source->convertTo(mMatF, CV_32FC3, alpha);

            //Put background pixel into mat for conversion
            mBackground = desc.backgroundPoint;
            cv::Mat backgroundPxMat(1,1,CV_32FC3, &mBackground);

            //Convert to appropriate colour space (including background pixel):
            mColourSpace = desc.targetColourspace;

            switch(desc.targetColourspace)
            {
            case InputAssemblerDescriptor::ETCS_RGB:
                break;
            case InputAssemblerDescriptor::ETCS_HSV:
                cv::cvtColor(mMatF, mMatF, CV_RGB2HSV);
                cv::cvtColor(backgroundPxMat, backgroundPxMat, CV_RGB2HSV);

                //Normalise hue:
                for(int y = 0; y < mMatF.cols; ++y)
                    for(int x = 0; x < mMatF.rows; ++x)
                    {
                        cv::Point3f& p = mMatF.at<cv::Point3f>(x,y);
                        p.x/=360.f;
                    }
                backgroundPxMat.at<cv::Point3f>(0,0).x/=360.f;
                break;
            case InputAssemblerDescriptor::ETCS_LAB:
                cv::cvtColor(mMatF, mMatF, CV_RGB2Lab);
                cv::cvtColor(backgroundPxMat, backgroundPxMat, CV_RGB2Lab);
                for(int y = 0; y < mMatF.cols; ++y)
                    for(int x = 0; x < mMatF.rows; ++x)
                    {
                        cv::Point3f& p = mMatF.at<cv::Point3f>(x,y);
                        p = cv::Point3f(p.x/100.f, (p.y+127.f)/254.f, (p.z+127.f)/254.f);
                    }

                mBackground = (math::vec3(mBackground.x/100.f,
                                          (mBackground.y+127.f)/254.f,
                                          (mBackground.z+127.f)/254.f));
                break;
            }

            mPoints = ProcessPoints(mMatF, desc.ipd);

            END_TIMER(ProcessingInput);
        }

        cv::Point3f InputAssembler::debugGetPointColour(math::vec3 p) const
        {
            switch(mColourSpace)
            {
            case InputAssemblerDescriptor::ETCS_RGB:
                break;
            case InputAssemblerDescriptor::ETCS_HSV:
            {
                p.x *= 360.f;
                cv::Mat mat(1,1,CV_32FC3, &p.x);
                cv::cvtColor(mat, mat, CV_HSV2RGB);
            }
                break;
            case InputAssemblerDescriptor::ETCS_LAB:
            {
                p = math::vec3(p.x*100.f,
                                p.y*254.f - 127.f,
                                p.z*254.f - 127.f);

                cv::Mat mat(1,1,CV_32FC3, &p.x);
                cv::cvtColor(mat, mat, CV_Lab2RGB);
            }
                break;
            default:
                assert(0);
            }

            return cv::Point3f(p.x,p.y,p.z);
        }

        const std::vector<math::vec3> &InputAssembler::points() const
        {
            return mPoints;
        }

        const cv::Mat& InputAssembler::mat() const
        {
            return mMatF;
        }

        math::vec3 InputAssembler::background() const
        {
            return mBackground;
        }
    }
}