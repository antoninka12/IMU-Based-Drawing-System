#pragma once

class LowPassFilter
{
public:
    explicit LowPassFilter(float alpha)
        : alpha_(alpha)
    {
    }

    float update(float input)
    {
        if (!initialized_) {
            output_ = input;
            initialized_ = true;
            return output_;
        }

        output_ = output_ + alpha_ * (input - output_);

        return output_;
    }

private:
    float alpha_;
    float output_ = 0.0f;
    bool initialized_ = false;
};