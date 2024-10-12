#include "Encoding.h"
#include "Configs.h"
#include "Common.h"

#include "Types/SimulationState.h"
#include "Types/Vector.h"
#include "Types/Ant.h"

int main() {
	/*
		Most known:
		RRLLLRLLLRRR Creates a filled triangle shape
		LLRRRLRLRLLR Creates a convoluted highway
		LRRRRRLLR  Fills space in a square around itself
		LLRR Grows symmetrically
		RLR Grows chaotically
		RL Default Langton's ant
		
		Others:
		RURLRU Expands chaotically quickly
		RLLLRLRRLLRRR Similar to LRRRRRLLR but slower and slows down growth considerably at iteration 8477782376

		L45R45 Expands very slowly
		L45UR45U Similar to L45R45, can be chained
		
		RLRRLRRLRRLR & RLC When initializing both ants at the center (+/- 1 pixel tolerance on the y axis) with the same direction a prism with complex patterns is made
	*/

	Vector2<SizeType> CanvasSize(65535, 65535);
	auto Center = CanvasSize / Vector2(2, 2);

	SimulationState          Simulation = {};
	Encoding::PaletteManager Palette = {};
	Encoding::EncoderState   Encoder = {};
	
	Simulation.Resize(CanvasSize);

	Simulation.AddAnt(Ant(Center, Vector2<int8_t>(0, 1), Configs::ParseStateMachine("RLRRLRRLRRLR"), true));
	Simulation.AddAnt(Ant(Center, Vector2<int8_t>(0, 1), Configs::ParseStateMachine("RLC"), true));

	Simulation.Reset();
	Palette.ResizePalette(Simulation.PossibleStates);
	
	Encoder.Threads.ThreadCount = 32;
	Encoder.Format = Encoding::ImageFormat::PNG_PALETTE;

	// ffmpeg -r 60 -i "Frames/%d.png" -b:v 5M -c:v libx264 -preset veryslow -qp 0 output.mp4
	// ffmpeg -r 60 -i "Frames/%d.png" -b:v 5M -c:v libx264 -preset veryslow -qp 0 -s 1920x1920 output.mp4
	// ffmpeg -r 60 -i "Frames/%d.png" -b:v 5M -c:v libx264 -preset veryslow -qp 0 -s 1920x1920 -sws_flags neighbor output.mp4
	// ffmpeg -r 30 -i "Frames/%d.png" -c:v libx264 -preset veryslow -qp 0 -s 7680x4320 output.mp4
	// ./LangtonsAnt | ~/ffmpeg/ffmpeg -y -f rawvideo -pix_fmt rgb24 -s 30720x30720 -r 30 -i - -c:v libx264 -preset veryslow -s 7680x7680 output.h264
	// ./LangtonsAnt | ~/ffmpeg/ffmpeg -y -f rawvideo -pix_fmt rgb24 -s 1920x1920 -r 30 -i - -c:v libx264 -preset veryslow -s 1920x1920 output.h264

	std::cout << "Ant state machines:\n";
	for (auto& Ant : Simulation.TemplateAnts)
		std::cout << '\t' << StateMachineToString(Ant.StateMachine) << '\n';
	

	size_t Iterations = 32ull * 1000000000ull;
	double FrameRate = 128.0; // Video frame rate
	double Time = 1.0; // Video time
	size_t Frames = size_t(Time * FrameRate);
	
	size_t CaptureDelta = size_t(double(Iterations) / double(Frames));

	for (size_t i = 0; i < Frames; i++) {
		size_t Result = Simulation.Simulate(CaptureDelta);

		printf("Frame: %lu Iters: %lu/%lu Threads: %lu\n", i, Result, CaptureDelta, Encoder.Threads.ActiveThreads());
		
		Encoder.EncodeAsync(Simulation, [&, i](const std::vector<uint8_t>& ImageData, const Vector2<int>&, unsigned int) {
			lodepng::save_file(ImageData, "frames/" + std::to_string(i) + ".png");
		});
	}
}