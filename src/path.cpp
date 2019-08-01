#include "../include/path.h"

#include <boost/filesystem.hpp>

manta::Path::Path(const std::string &path) {
	m_path = nullptr;
	setPath(path);
}

manta::Path::Path(const char *path) {
	m_path = nullptr;
	setPath(std::string(path));
}

manta::Path::Path(const Path &path) {
	m_path = new boost::filesystem::path;
	*m_path = path.getBoostPath();
}

manta::Path::Path() {
	m_path = nullptr;
}

manta::Path::Path(const boost::filesystem::path &path) {
	m_path = new boost::filesystem::path;
	*m_path = path;
}

manta::Path::~Path() {
	delete m_path;
}

std::string manta::Path::toString() const {
	return m_path->string();
}

void manta::Path::setPath(const std::string &path) {
	if (m_path != nullptr) delete m_path;

	m_path = new boost::filesystem::path(path);
}

bool manta::Path::operator==(const Path &path) const {
	return boost::filesystem::equivalent(this->getBoostPath(), path.getBoostPath());
}

manta::Path manta::Path::append(const Path &path) const {
	return Path(getBoostPath() / path.getBoostPath());
}

void manta::Path::getParentPath(Path *path) const {
	path->m_path = new boost::filesystem::path;
	*path->m_path = m_path->parent_path();
}

const manta::Path &manta::Path::operator=(const Path &b) {
	if (m_path != nullptr) delete m_path;

	m_path = new boost::filesystem::path;
	*m_path = b.getBoostPath();

	return *this;
}

std::string manta::Path::getExtension() const {
	return m_path->extension().string();
}

std::string manta::Path::getStem() const {
	return m_path->stem().string();
}

bool manta::Path::isAbsolute() const {
	return m_path->is_complete();
}

bool manta::Path::exists() const {
	return boost::filesystem::exists(*m_path);
}
