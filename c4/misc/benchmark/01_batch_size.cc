// run_seconds,batch_size,iterations_per_run,device
// 0.135,  1,50,mps
// 0.140,  2,50,mps
// 0.140,  4,50,mps
// 0.144,  8,50,mps
// 0.161, 16,50,mps
// 0.217, 32,50,mps
// 0.453, 64,50,mps
// 0.804,128,50,mps
#include <torch/script.h>

#include <torch/mps.h>
#include <iostream>
#include <memory>

const auto deviceToUse = torch::kMPS;

template<class T>
void run_with_batch_size(T& m, int batch_size)  {
        c10::InferenceMode guard;

        std::vector<torch::jit::IValue> inputs;
        auto opt = torch::TensorOptions()
                .dtype(torch::kFloat32)
                .device(deviceToUse);

        inputs.push_back(torch::randn({batch_size, 3, 6, 7}, opt));

        // warm up
        auto count = 10;
        for (int i = 0; i < count; i++) {
                m.forward(inputs);
        }

        // real run
        count = 50;

        torch::mps::synchronize();
        auto start = std::chrono::system_clock::now();
        for (int i = 0; i < count; i++) {
                m.forward(inputs);
        }

        torch::mps::synchronize();
        auto end = std::chrono::system_clock::now();

        std::chrono::duration<double, std::milli> fp_ms = end - start;

        std::cout
                << std::setw(5) << std::fixed << std::setprecision(3)
                << fp_ms.count() / 1000 << ","
                << std::setw(3) << batch_size << ","
                << count << ",mps\n";
}

int main(int argc, const char* argv[]) {
        if (argc != 2) {
                std::cerr << "usage: ./a.out <path-to-script-module>\n";
                return -1;
        }



        torch::jit::script::Module module;
        try {
                module = torch::jit::load(argv[1]);
                module.to(deviceToUse);
                module.eval();

                std::cout << "run_seconds,batch_size,iterations_per_run,device\n";
                for (auto bs: {1, 2, 4, 8, 16, 32, 64, 128})
                        run_with_batch_size(module, bs);

        } catch (const c10::Error& e) {
                std::cerr << "error loading the model\n";
                std::cerr << e.msg();
                return -1;
        }

        std::cout << "ok\n";
}
