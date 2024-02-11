#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iomanip> // for cout precision
#include <ostream>
#include <string>
#include <vector>

const int NUM_LIGHT_BODIES = 20000; // TOFIX: make it easier for others to change
const int NUM_DARK_BODIES = 20000;

// -----------------------------------------------------------------------------------------------------------------------------
// structs for holding data

// 3D point
struct Cartesian3D {
	double x;
	double y;
	double z;
};

std::ostream& operator<<(std::ostream& out, const Cartesian3D& loc) {
	out << "( " << loc.x << ", " << loc.y << ", " << loc.z << " )";
	return out;
}

// body
struct Body {
	bool dark; // baryonic matter / dark matter
	Cartesian3D loc; // location of the body
	double mass; // mass of the body
};

std::ostream& operator<<(std::ostream& out, const Body& body) {
	out << "Dark Matter: " << body.dark << ", Location: " << body.loc << ", Mass: " << body.mass;
	return out;
}

// histogram bin
struct Bin {
	double r_min; // minimum radial distance from galactic center of mass
	double r_max; // maximum radial distance
	int count; // body count
	double total_mass; // total mass in this bin
	double r; // distance used for this bin's area/volume/density calculations
	double a; // shell area
	double v; // shell volume
	double p; // density
};

std::ostream& operator<<(std::ostream& out, const Bin& bin) {
	out << "Range: [" << bin.r_min << ", " << bin.r_max << "), Count: " << bin.count << ", Total Mass: " << bin.total_mass << ", r = " << bin.r << ", a = " << bin.a << ", v = " << bin.v << ", p = " << bin.p;
	return out;
}

// -----------------------------------------------------------------------------------------------------------------------------
// for debugging
const bool debug = false;
const bool verbose = true;

std::ostream& operator<<(std::ostream& out, const std::vector<Bin*>& hash_array) {
	int size = hash_array.size();
	for (int i = 0; i < size; ++i) {
		if (i != 0)
			out << "\n";
		out << "Bin " << i << ": " << *(hash_array[i]);
	}
	return out;
}

// -----------------------------------------------------------------------------------------------------------------------------
// I/O

// converts string to double from input file
double get_coord(std::ifstream& input_file) {
	// getting string
	std::string tok;
	input_file >> tok;
	
	// converting to double
	// token = token.substr(0, token.length() - 1); // TOREMOVE: useless? (stod takes care of the comma anyways)
	double num = std::stod(tok); // actually converting
	if (debug and verbose) {
		std::cerr << "Debug: tok: " << tok << "\n";
		std::cerr << "Debug: num: " << num << "\n";
	}
	
	return num;
}

// reading data from input file
void read_data(const std::string& input_filename, const bool& dark_only, Cartesian3D& COM, Body* bodies, const int& size) {
	// opening file
	std::ifstream input_file(input_filename);
	if (!input_file.good()) {
		std::cerr << "ERROR: Unable to read \"" << input_filename << "\"!" << std::endl;
		exit(1);
	}
	std::cerr << "Reading \"" << input_filename << "\"...\n";

	// getting rid of unused lines
	std::string line;
	std::getline(input_file, line); // <bodies> (or "cartesian    = 0" depending on the input file)
	if (line[0] == '<')
		std::getline(input_file, line); // cartesian    = 0
	std::getline(input_file, line); // lbr & xyz    = 1
	
	// confirming null potential
	std::string tok;
	input_file >> tok; // hasMilkyway
	input_file >> tok; // =
	input_file >> tok; // 0/1
	if (std::stoi(tok) == 1)
		std::cerr << "Warning: Milky Way potential detected.\n";

	// getting center of mass
	std::cerr << std::setprecision(16); // increasing precision of doubles in cerr
	input_file >> tok; // centerOfMass
	input_file >> tok; // =
	COM.x = get_coord(input_file);
	COM.y = get_coord(input_file);
	COM.z = get_coord(input_file);
	if (debug)
		std::cerr << "\nDebug: Center of Mass: " << COM << "\n";

	// getting rid of more unused lines
	std::getline(input_file, line); // centerOfMomentum = ...
	std::getline(input_file, line); // # ignore   id   x   y   z   l   b   r   v_x   v_y   v_z   mass   v_los
	
	// filling body array
	for (int i = 0; i < size; ++i) {
		Body* new_body = &(bodies[i]);
		input_file >> tok; // # ignore (baryonic/dark matter)
		new_body->dark = std::stoi(tok);
		
		// get only dark matter
		if (dark_only and dark_only != new_body->dark) { // this is baryonic matter
			std::getline(input_file, line); // clearing rest of line
			--i;
			continue; // skip
		}

		input_file >> tok; // unused: id
		
		input_file >> tok; // x
		new_body->loc.x = std::stod(tok);
		input_file >> tok; // y
		new_body->loc.y = std::stod(tok);
		input_file >> tok; // z
		new_body->loc.z = std::stod(tok);
		
		input_file >> tok; // unused: l
		input_file >> tok; // unused: b
		input_file >> tok; // unused: r
		input_file >> tok; // unused: v_x
		input_file >> tok; // unused: v_y
		input_file >> tok; // unused: v_z
		
		input_file >> tok; // mass
		new_body->mass = std::stod(tok);

		std::getline(input_file, line); // clearing rest of line

		if (i < 5)
			std::cerr << "bodies[" << i << "]: " << *new_body << "\n";
		else if (debug and verbose)
			std::cerr << "Debug: bodies[" << i << "]: " << *new_body << "\n";
	}
	if (!debug)
		std::cerr << "...\n";
	std::cerr << "\n" << size << " bodies read.\n";

	// done reading; closing input file
	input_file.close();
}

// writing out to csv file
void output_hist(const std::string& output_filename, const std::vector<Bin*>& hash_array) {
	// opening output file
	std::ofstream output_file(output_filename);
	if (!output_file.good()) {
		std::cerr << "ERROR: Unable to write to \"" << output_filename << "\"!" << std::endl;
		exit(1);
	}

	// writing to output file
	std::cerr << "\nWriting to \"" << output_filename << "\"...\n";
	output_file << "#, r_min (kpc), r_max (kpc), count, total mass (SMU), r (kpc), a (kpc^2), v (kpc^3), p (SMU/kpc^3)\n";
	int size = hash_array.size();
	for (int i = 0; i < size; ++i) {
		Bin* bin = hash_array[i];
		output_file << (i+1) << ", ";
		output_file << bin->r_min << ", ";
		output_file << bin->r_max << ", ";
		output_file << bin->count << ", ";
		output_file << bin->total_mass << ", ";
		output_file << bin->r << ", ";
		output_file << bin->a << ", ";
		output_file << bin->v << ", ";
		output_file << bin->p << "\n";
	}

	output_file.close(); // done writing; close
}

// -----------------------------------------------------------------------------------------------------------------------------
// processing data from input file

// counting bodies into bins
void count_bodies(const double& dr, const Cartesian3D& COM, Body* bodies, const int& size, std::vector<Bin*>& hash_array) {
	for (int i = 0; i < size; ++i) {
		Body* body = &(bodies[i]);

		// calculating radial distance and bin index
		double r = sqrt(pow(COM.x - body->loc.x, 2) + pow(COM.y - body->loc.y, 2) + pow(COM.z - body->loc.z, 2));
		int bin_i = r / dr;
		if (debug and verbose) {
			std::cerr << "Debug: r: " << r << "\n";
			std::cerr << "Debug: bin_i: " << bin_i << "\n";
		}

		// adding to bin
		int max_i = hash_array.size() - 1; // highest current index in the hash array
		while (max_i < bin_i) { // adding new bins
			Bin* new_bin = new Bin;
			++max_i;

			// initializing bin
			new_bin->r_min = max_i * dr;
			new_bin->r_max = new_bin->r_min + dr;
			new_bin->count = 0;
			new_bin->total_mass = 0.0;
			new_bin->r = 0.0;
			new_bin->a = 0.0;
			new_bin->v = 0.0;
			new_bin->p = 0.0;

			hash_array.push_back(new_bin);
			if (debug and verbose)
				std::cerr << "Debug: New bin (" << max_i << ")\n";
		}
		hash_array[bin_i]->count++; // incrementing count
		hash_array[bin_i]->total_mass += body->mass; // incrementing total mass
		if (debug and verbose) {
			std::cerr << "Debug: new count: " << hash_array[bin_i]->count << "\n";
			std::cerr << "Debug: new total mass: " << hash_array[bin_i]->total_mass << "\n\n";
		}
	}

	// checking process
	if (debug) { 
		if (verbose)
			std::cerr << "Debug: hash_array:\n" << hash_array << "\n";

		// counting
		int num_bodies = 0;
		int arr_size = hash_array.size();
		for (int i = 0; i < arr_size; ++i)
			num_bodies += hash_array[i]->count;
		std::cerr << "Debug: Checking...\n";
		assert(num_bodies == size);
		std::cerr << "Debug: Good!\n";
	}
}

// calculates the density for each shell
void densify(const double& dr, std::vector<Bin*>& hash_array) {
	int size = hash_array.size();
	for (int i = 0; i < size; ++i) {
		Bin* bin = hash_array[i]; // getting bin

		// calculating volume
		double r = dr * i + dr / 2; // center of bin
		double a = 4 * M_PI * pow(r, 2);
		double v = a * dr;
		
		// calculating density
		double p = bin->total_mass / v;

		// saving values
		bin->r = r;
		bin->a = a;
		bin->v = v;
		bin->p = p;
	}

	if (debug and verbose)
		std::cerr << "Debug: hash_array (density):\n" << hash_array << "\n";
}

// -----------------------------------------------------------------------------------------------------------------------------
int main(int argc, char** argv) {
	// input checking
	if (argc != 5) {
		std::cerr << "ERROR: Invalid inputs\n";
		std::cerr << "Usage: ./density_calculator.exe <input file (.out file)> <dr> <dark_matter (1) | all matter (0)> <density (1) | count (0)>" << std::endl;
		return 1;
	}

	// getting inputs
	const std::string input_filename = argv[1];
	const double dr = std::stod(argv[2]);
	const bool dark_only = std::stoi(argv[3]);
	const bool density = std::stoi(argv[4]);
	if (debug) {
		std::cerr << "Debug: input_filename: " << input_filename << "\n";
		std::cerr << "Debug: dr: " << dr << "\n";
		std::cerr << "Debug: dark_only: " << dark_only << "\n";
		std::cerr << "Debug: density: " << density << "\n\n";
	}

	// data storage
	Cartesian3D COM; // center of mass
	Body* bodies;
	int size = NUM_DARK_BODIES;
	if (!dark_only)
		size += NUM_LIGHT_BODIES;
	bodies = new Body[size];

	// data reading/processing
	read_data(input_filename, dark_only, COM, bodies, size);

	// processing data
	std::vector<Bin*>* hash_array = new std::vector<Bin*>();
	std::cerr << "Processing data...\n";
	count_bodies(dr, COM, bodies, size, *hash_array);
	if (density)
		densify(dr, *hash_array);

	// writing out to file
	size_t i = input_filename.rfind('.');
	std::string input_filetoken = input_filename.substr(0, i); // getting file name without .out ending
	i = input_filetoken.rfind('/');
	if (i != std::string::npos)
		input_filetoken = input_filetoken.substr(i + 1, input_filetoken.size() - (i + 1));
	std::string output_filename = "output/" + input_filetoken + "_dr" + std::string(argv[2]) + "_dm" + std::string(argv[3]) + ".csv"; // generating new file name
	
	if (debug and verbose)
		std::cerr << "Debug: output_filename: " << output_filename << "\n";
	
	output_hist(output_filename, *hash_array);

	// memory management
	delete bodies;
	int arr_size = hash_array->size();
	for (int i = 0; i < arr_size; ++i)
		delete (*hash_array)[i];
	delete hash_array;

	// successful process
	std::cerr << "Done!" << std::endl;
	std::cout << arr_size << std::endl; // returning to shell
	return 0;
}