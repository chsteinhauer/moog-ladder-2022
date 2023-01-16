
#pragma once
#include <JuceHeader.h>
#include <vector>

/*
 * CircularBuffer stores the 3 most recent values for each stage, consisting of a
 * vector of stages (Stage), which are individually responsible for managing the 
 * values of a stage.
 */

class CircularCache {
public:
    CircularCache(int order = 4) {

        for (int i = 0; i < order; i++) {
            //Stage stage;
            stages.push_back(Stage());

            // For initial base case (for the recursion), set cache(-1) = 0 in buffer
            stages.at(i).set(-1, 0.f);
        }
    }

    float get(int n, int stage)
    {
        return stages.at(stage).get(n);
    }

    void set(int n, int stage, float val)
    {
        stages.at(stage).set(n, val);
    }

    bool has(int n, int stage)
    {
        return stages.at(stage).has(n);
    }

    void reset() {
        for (Stage s : stages)
            s.reset();

        stages.clear();
    }


private:
    struct Stage {

        Stage() {
            for (int i = 0; i < bsize; i++) {
                values.push_back(0.f);
            }
        }

        float get(int n) {
            int i = getIndex(n);

            return *values.at(i);
        }

        void set(int n, float val)
        {
            int i = getIndex(n);
            int next = getIndex(n + 1);

            values.at(i).emplace(val);
            values.at(next) = juce::Nullopt::Nullopt(0);
        }

        bool has(int n) {
            int i = getIndex(n);

            return values.at(i).hasValue();
        }

        int getIndex(int n) {
            int i = 0;

            if (n < 0) {
                i = bsize - (-n % bsize);
            }
            else {
                i = n % bsize;
            }

            return i;
        }

        void reset() {
            values.clear();
        }

        const int bsize = 4;

        std::vector<juce::Optional<float>> values;
    };

    std::vector<Stage> stages;
};

