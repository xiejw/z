// See https://pytorch.org/tutorials/advanced/cpp_export.html
#include <torch/script.h>

#include <iostream>
#include <memory>

// macOS mps
const auto deviceToUse = torch::kMPS;
const auto deviceCPU   = torch::kCPU;

int main(int argc, const char* argv[]) {
        if (argc != 2) {
                std::cerr << "usage: ./a.out <path-to-script-module>\n";
                return -1;
        }

        c10::InferenceMode guard;

        torch::jit::script::Module module;
        try {
                module = torch::jit::load(argv[1]);
                module.to(deviceToUse);
                module.eval();

                std::vector<torch::jit::IValue> inputs;
                // here to reproduce the result, we generate the result on CPU
                // first and then move to mps.
                auto opt = torch::TensorOptions()
                        .dtype(torch::kFloat32)
                        .device(deviceCPU);

                torch::manual_seed(123);
                inputs.push_back(torch::randn({1, 3, 6, 7}, opt).to(deviceToUse));
                auto output = module.forward(inputs);
                torch::Tensor t0 = output.toTuple()->elements()[0].toTensor();
                torch::Tensor t1 = output.toTuple()->elements()[1].toTensor();

                std::cout << t0 << std::endl;
                std::cout << t0.cpu().data_ptr<float>()[0] << std::endl;
                std::cout << t1 << std::endl;
                std::cout << t1.cpu().data_ptr<float>()[0] << std::endl;

        } catch (const c10::Error& e) {
                std::cerr << "error loading the model\n";
                std::cerr << e.msg();
                return -1;
        }

        std::cout << "ok\n";
}
