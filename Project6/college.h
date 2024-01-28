#ifndef COLLEGE_H
#define COLLEGE_H

#include <map>
#include <set>
#include <vector>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <string_view>
#include <utility>


class Course {
 private:
  const std::string name;
  bool active;
  friend class College;

 public:
  Course(const std::string& name, bool active = true)
      : name(name), active(active) {}
  
  ~Course() = default;

  const std::string& get_name() const noexcept{ return name; }

  bool is_active() const noexcept { return active; }
};

class Person {
 private:
  const std::string name;
  const std::string surname;

 public:
  Person(const std::string& name, const std::string& surname)
      : name(name), surname(surname) {}

  virtual ~Person() = default;

  const std::string& get_name() const noexcept { return name; }

  const std::string& get_surname() const noexcept { return surname; }
};

namespace comp {
  // string comparison operators are noexcept
  // shared_ptr operator-> is noexcept
template <typename T>
requires std::is_same_v<T, Course> or std::is_same_v<T, const Course>
struct CourseComparator {
  bool operator()(const std::shared_ptr<T>& a,
                  const std::shared_ptr<T>& b) const noexcept{
    return a->get_name() < b->get_name();
  }
};

struct PersonComparator {
  bool operator()(const std::shared_ptr<Person>& a,
                  const std::shared_ptr<Person>& b) const noexcept {
    if (a->get_surname() != b->get_surname()) {
      return a->get_surname() < b->get_surname();
    }
    return a->get_name() < b->get_name();
  }
};

}; // end namespace comp

class Student : virtual public Person {
 private:
  using courses_t = std::set<std::shared_ptr<const Course>, 
                                comp::CourseComparator<const Course>>;
  courses_t courses;
  bool active;
  friend class College;

 public:
  Student(std::string name, std::string surname, bool active = true)
      : Person(name, surname), active(active) {}

  bool is_active() const noexcept { return active; }

  const courses_t& get_courses() const { return courses; }

  virtual ~Student() = default;
};

class Teacher : virtual public Person {
 private:
  using courses_t = std::set<std::shared_ptr<const Course>, 
                                  comp::CourseComparator<const Course>>;
  courses_t courses;
  friend class College;

 public:
  Teacher(std::string name, std::string surname) : Person(name, surname) {}

  const courses_t& get_courses() const noexcept { return courses; }
  virtual ~Teacher() = default;  

};

class PhDStudent : public Student, public Teacher {
 private:
  friend class College;

 public:
  PhDStudent(std::string name, std::string surname, bool active = true)
      : Person(name, surname),
        Student(name, surname, active),
        Teacher(name, surname) {}

  virtual ~PhDStudent() = default;
};

namespace concepts {
  template <typename T>
  concept IsStudentOrTeacher = std::is_same_v<T, Student> or 
                                          std::is_same_v<T, Teacher>;
};



//              Short Note About the Solution:

// The College class encompasses a set of all people and features a map of
// courses. Each course in the map is associated with a pair of two sets:
// - The first set represents the students enrolled in the course.
// - The second set represents the teachers leading the course.

// Memory leaks are prevented, and circular references are absent. College
// instances maintain pointers to people and courses. Students, teachers, and
// PhD students have pointers to courses.

// Several methods that take std::shared_ptr<Course> have been overloaded to
// also accept std::shared_ptr<const Course>. This is necessary because the
// get_method for students and teachers returns a set of 
// shared_ptr<const Course> to meet the requirement of unmodifiablity.
// Overloading these methods accommodates users who
// pass courses obtained from the above method into other functions,
// (This is particularly reasonable if you intend for student B to enroll in
// the same courses as student A.)




class College {
 private:
  using students_t =
      std::set<std::shared_ptr<Student>, comp::PersonComparator>;
  using teachers_t =
      std::set<std::shared_ptr<Teacher>, comp::PersonComparator>;

  using courses_t =
      std::map<std::shared_ptr<Course>, std::pair<students_t, teachers_t>,
               comp::CourseComparator<Course>>;
  using people_t = std::set<std::shared_ptr<Person>, comp::PersonComparator>;

  people_t people;
  courses_t courses;


  //std::string_view::substr is noexcept (if starting pos<= size())
  // it is the case in this code
  bool match(std::string_view check, std::string_view pattern) const noexcept {
    if (check.empty() and pattern.empty()) return true;

    if (pattern.empty()) return false;

    if (pattern[0] == '*') {
      for (size_t i = 0; i <= check.size(); ++i) {
        if (match(check.substr(i), pattern.substr(1))) return true;
      }
    }

    if (!check.empty() and (pattern[0] == '?' or pattern[0] == check[0])) {
      if (match(check.substr(1), pattern.substr(1))) return true;
    }
    return false;
  }


 public:
  College() = default;

  bool add_course(const std::string& name, bool active = true) {
    bool inserted = courses.emplace(std::make_shared<Course>(name, active),
                              std::pair{students_t{}, teachers_t{}}).second;
    return inserted;
  }
  
  // In match function implicit string to string_view conversion
  // is not noexcept
  auto find_courses(const std::string& pattern) const {
    std::vector<std::shared_ptr<Course>> filtered_courses;
    for (const auto& course : courses) {
      if (match(course.first->get_name(), pattern)) {
        filtered_courses.push_back(course.first);
      }
    }
    return filtered_courses;
  }
  

  bool change_course_activeness(const std::shared_ptr<Course>& course,
                                bool active) noexcept {

    // Since CourseCompartor::operator< is noexcept
    // find method is noexcept
    auto it = courses.find(course);
    if (it != courses.end() and it->first == course ) {
      // iterator it is valid hence
      // -> should not fail and throw 
      it->first->active = active;
      return true;
    }
    return false;
  }

  bool change_course_activeness(const std::shared_ptr<const Course>& course,
                                bool active) noexcept {
    std::shared_ptr<Course> nonconst = std::const_pointer_cast<Course>(course);
    return change_course_activeness(nonconst, active);
  }

  bool remove_course(const std::shared_ptr<Course>& course) noexcept {
    // Since CourseCompartor::operator< is noexcept
    // find method is noexcept
    auto it = courses.find(course);
    if (it != courses.end()) {
      // iterator it is valid hence
      // -> should not fail and throw 
      it->first->active = false;
      courses.erase(it);
      return true;
    } else return false;
  }
  bool remove_course(const std::shared_ptr<const Course>& course) noexcept {
    std::shared_ptr<Course> nonconst = std::const_pointer_cast<Course>(course);
    return remove_course(nonconst);
  }

  template <typename T>
  requires concepts::IsStudentOrTeacher<T> or std::is_same_v<T, PhDStudent>
  bool add_person(const std::string& name, const std::string& surname,
                                                        bool active = true) {

    if constexpr (std::is_same_v<T, Teacher>) {
      bool inserted = people.insert(std::make_shared<T>(name, surname)).second;
      return inserted;
    } else {
      bool inserted =
          people.insert(std::make_shared<T>(name, surname, active)).second;
      return inserted;
    }
  }

  bool change_student_activeness(const std::shared_ptr<Student>& student,
                                 bool active) noexcept {
    
    // PersonComparator::operator< is noexcept
    // hence find method is noexcept
    auto it = people.find(student);

    if (it != people.end() and *it == student) {
      // std::dynamic_pointer_cast is noexcept
      // We need it because people contain students + teacher
      // hence the found person may not be a student 
      std::shared_ptr<Student> s = std::dynamic_pointer_cast<Student>((*it));
      if (s != nullptr) {
        s->active = active;
        return true;
      }
    }
    return false;
  }

  template <typename T>
  requires std::derived_from<T, Person>
  auto find(const std::string& name_pattern, 
                  const std::string& surname_pattern) const {

    std::vector<std::shared_ptr<T>> filtered_people;
    for (const auto& person : people) {
      bool name_matched = match(person->get_name(), name_pattern);
      bool surname_matched = match(person->get_surname(), surname_pattern);
      if (name_matched and surname_matched) {
        std::shared_ptr<T> temp = std::dynamic_pointer_cast<T>(person);

        if (temp != nullptr) filtered_people.push_back(temp);        
      }
    }
    return filtered_people;
  }
  
  // obviously it is const since we return auto (hence we return a copy)
  // cannot be noexcept since if a course is not found
  // the empty set is returned, allocation may throw
  template <typename T>
  requires concepts::IsStudentOrTeacher<T>
  auto find(const std::shared_ptr<Course>& course) const {
    
    auto it = courses.find(course);

    if constexpr (std::is_same_v<T, Student>) {
      if (it == courses.end() or it->first != course) return students_t{};
      return it->second.first;
      
    }

    else if constexpr (std::is_same_v<T, Teacher>) {
      if (it == courses.end() or it->first != course) return teachers_t{};
      return it->second.second;
    }
  }
  
  template <typename T>
  requires concepts::IsStudentOrTeacher<T>
  auto find(const std::shared_ptr<const Course>& course) const {
    std::shared_ptr<Course> nonconst = std::const_pointer_cast<Course>(course);
    return find<T>(nonconst);
  }

  template <typename T>
  requires concepts::IsStudentOrTeacher<T>
  bool assign_course(const std::shared_ptr<T>& person, 
                          const std::shared_ptr<Course>& course) {
    
    if (!course->is_active()) {
      throw std::logic_error("Incorrect operation on an inactive course.");
    }

    auto course_it = courses.find(course);

    if (course_it == courses.end() or course_it->first != course) {
      throw std::logic_error("Non-existing course.");
    }

    auto person_it = people.find(person);

    if (person_it == people.end() or *person_it != person) {
      throw std::logic_error("Non-existing person.");
    }

    if constexpr (std::is_same_v<T, Student>) {
      if (!person->is_active()) {
        throw std::logic_error("Incorrect operation for an inactive student.");
      }
      bool inserted = person->courses.insert(course).second;
      course_it->second.first.insert(person);
      return inserted;
    } else {
      bool inserted = person->courses.insert(course).second;
      course_it->second.second.insert(person);
      return inserted;
    }
  }
  
  template <typename T>
  requires concepts::IsStudentOrTeacher<T>
  bool assign_course(const std::shared_ptr<T>& person, 
                          const std::shared_ptr<const Course>& course) {
    std::shared_ptr<Course> nonconst = std::const_pointer_cast<Course>(course);
    return assign_course<T>(person, nonconst);
  }
};

#endif //end header_guard 
